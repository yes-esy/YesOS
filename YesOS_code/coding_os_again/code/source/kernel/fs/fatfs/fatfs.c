/**
 * @FilePath     : /code/source/kernel/fs/fatfs/fatfs.c
 * @Description  :  fat文件系统的实现.c文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-12 14:05:13
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "fs/fatfs/fatfs.h"
#include "fs/fs.h"
#include "dev/dev.h"
#include "tools/log.h"
#include "core/memory.h"
#include "tools/klib.h"
#include "sys/fcntl.h"
static int expand_file(file_t *file, int incr_bytes); // 声明
/**
 * @brief        : 检查簇是否有效
 * @param         {cluster_t} cluster: 簇编号
 * @return        {int} 有效返回1,无效返回0
 **/
int cluster_is_valid(cluster_t cluster)
{
    return (cluster < FAT_CLUSTER_INVALID) && (cluster >= 0x2);
}
/**
 * @brief        : 挂载操作,进行某些初始化
 * @param         {_fs_t} *fs: 指定的文件系统的指针
 * @param         {int} major: 主设备号
 * @param         {int} minor: 次设备号
 * @return        {int} : 成功返回0,失败返回-1
 **/
int fatfs_mount(struct _fs_t *fs, int major, int minor)
{
    int dev_id = dev_open(major, minor, (void *)0); // 挂载设备
    if (dev_id < 0)                                 // 挂载失败
    {
        log_printf("mount fat file system failed, major:%x, minor:%x", major, minor);
        return -1;
    }
    dbr_t *dbr = (dbr_t *)memory_alloc_page(); // 分配一页内存
    if (dbr == (dbr_t *)0)
    {
        log_printf("alloc memory page failed.\n");
        goto mount_failed;
    }
    int err = dev_read(dev_id, 0, (char *)dbr, 1); // 读取dbr
    if (err < 1)
    {
        log_printf("read dbr failed.\n");
        goto mount_failed;
    }
    // 初始化
    fat_t *fat = &fs->fat_data;
    fat->curr_sector = -1;                                                    // 当前扇区为-1
    fat->fat_buf = (uint8_t *)dbr;                                            // 指向分配的区域
    fat->bytes_per_sec = dbr->BPB_BytsPerSec;                                 // 扇区字节数
    fat->table_start = dbr->BPB_RsvdSecCnt;                                   // 起始扇区
    fat->table_sectors = dbr->BPB_FATSz16;                                    // 总扇区数量
    fat->table_cnt = dbr->BPB_NumFATs;                                        // 起始扇区
    fat->root_ent_cnt = dbr->BPB_RootEntCnt;                                  // 根目录数量
    fat->sec_per_cluster = dbr->BPB_SecPerClus;                               // 簇字节数
    fat->root_start = fat->table_start + fat->table_sectors * fat->table_cnt; // 根目录起始扇区
    fat->data_start = fat->root_start + fat->root_ent_cnt * 32 / SECTOR_SIZE; // 数据区起始扇区号
    fat->cluster_byte_size = fat->sec_per_cluster * dbr->BPB_BytsPerSec;      // 每个簇的大小
    fat->fs = fs;                                                             // 文件系统指针
    if (fat->table_cnt != 2)                                                  //
    {
        log_printf("%s:fat table error: major: %x, minor: %x\n", major, minor);
        goto mount_failed;
    }

    if (kernel_memcmp(dbr->BS_FileSysType, "FAT16", 5) != 0) // 非FAT16系统
    {
        log_printf("not a fat file system, major: %x, minor: %x\n", major, minor);
        goto mount_failed;
    }

    fs->type = FS_FAT16;        // 文件系统类型
    fs->data = &(fs->fat_data); // 数据
    fs->dev_id = dev_id;        // 设备号
    return 0;
mount_failed:
    if (dbr)
    {
        memory_free_page((uint32_t)dbr);
    }
    dev_close(dev_id);
    return -1;
}
/**
 * @brief        : 取消挂载
 * @param         {_fs_t} *fs: 指定的文件系统的指针
 * @return        {void}
 **/
void fatfs_unmount(struct _fs_t *fs)
{
    fat_t *fat = (fat_t *)fs->data;           // 获取fat指针
    dev_close(fs->dev_id);                    // 关闭设备
    memory_free_page((uint32_t)fat->fat_buf); // 释放dbr结构
    return;
}
/**
 * @brief        : 获取文件类型
 * @param         {diritem_t} *item: 文件目录项
 * @return        {file_type_t} : 返回文件类型结构体
 **/
file_type_t diritem_get_type(diritem_t *item)
{
    file_type_t type = FILE_UNKNOWN; // 默认为未知类型
    // volum_id或者隐藏返回未知
    if (item->DIR_Attr & (DIRITEM_ATTR_VOLUME_ID | DIRITEM_ATTR_HIDDEN | DIRITEM_ATTR_SYSTEM))
    {
        return FILE_UNKNOWN;
    }
    // 长文件名
    if ((item->DIR_Attr & DIRITEM_ATTR_LONG_NAME) == DIRITEM_ATTR_LONG_NAME)
    {
        return FILE_UNKNOWN;
    }

    return item->DIR_Attr & DIRITEM_ATTR_DIRECTORY ? FILE_DIR : FILE_NORMAL;
}
/**
 * @brief        : 从目录项中读取数据
 * @param         {fat_t *} fat: fat16文件系统描述符指针
 * @param         {file_t *} file: 数据读入的文件指针
 * @param         {diritem_t *} item: 目录项
 * @param         {int} index: 文件在目录中的索引
 * @return        {void}
 **/
static void read_from_diritem(fat_t *fat, file_t *file, diritem_t *item, int index)
{
    file->type = diritem_get_type(item);                                   // 设置类型
    file->pos = 0;                                                         // 读取位置
    file->f_index = index;                                                 // 文件位置
    file->size = (int)item->DIR_FileSize;                                  // 文件大小
    file->start_block = (item->DIR_FstClusHI << 16) | item->DIR_FstClusL0; // 起始簇号
    file->current_block = file->start_block;                               // 当前簇号
}
/**
 * @brief        : 转换成短的名称
 * @param         {char *} dest: 转换后的存储地址
 * @param         {char *} src: 需要转换的名称的地址
 * @return        {void}
 **/
static void to_sfn(char *dest, const char *src)
{
    kernel_memset(dest, ' ', SFN_LEN); // 清空目的存储空间
    char *curr = dest;                 // 起始存储地址
    char *end = dest + SFN_LEN;        // 终址
    while (*src && (curr < end))
    {
        char c = *src++; // 取出原字符
        switch (c)
        {
        case '.': // 遇到点跳转至扩展名
            curr = dest + 8;
            break;
        default:
            if ((c >= 'a') && (c <= 'z')) // 转换成大写
            {
                c = c - 'a' + 'A';
            }
            *curr++ = c; // 写入该字符
            break;
        }
    }
}
/**
 * @brief        : 目录表项与路径是否匹配
 * @param         {diritem_t *} item: 需要匹配的目录表项
 * @param         {char} *path: 需要匹配的路径
 * @return        {int} : 匹配返回1,否则返回0
 **/
int diritem_name_match(diritem_t *item, const char *path)
{
    char buf[SFN_LEN]; // 存储转换后的路径名称
    to_sfn(buf, path); // 转换成短的文件名
    return kernel_memcmp(buf, item->DIR_Name, SFN_LEN) == 0;
}
/**
 * @brief        : 读取磁盘数据,如果fat缓冲区保存了该扇区读取的数据直接返回
 * @param         {fat_t} *fat: fat文件系统描述符指针
 * @param         {int} sector: 要读取得扇区
 * @return        {int} : 读取成功返回0,失败返回-1
 **/
static int bread_sector(fat_t *fat, int sector)
{
    if (fat->curr_sector == sector) // 已经缓存该扇区
    {
        return 0;
    }
    int cnt = dev_read(fat->fs->dev_id, sector, fat->fat_buf, 1); // 不考虑缓存直接读取
    if (cnt == 1)                                                 // 读取失败
    {
        fat->curr_sector = sector; // 修改当前扇区
        return 0;
    }
    return -1;
}
/**
 * @brief        : 往磁盘写入数据
 * @param         {fat_t} *fat: fat文件系统描述符指针
 * @param         {int} sector: 要写入扇区
 * @return        {int} : 读取成功返回0,失败返回-1
 **/
static int bwrite_sector(fat_t *fat, int sector)
{
    int cnt = dev_write(fat->fs->dev_id, sector, fat->fat_buf, 1);
    return (cnt == 1) ? 0 : -1;
}
/**
 * @brief        : 在root目录中读取diritem
 * @param         {fat_t} *fat: fat文件系统描述符指针
 * @param         {int} index: 索引
 * @return        {diritem_t*} : 目录项指针
 **/
static diritem_t *read_dir_entry(fat_t *fat, int index)
{
    if ((index < 0) || (index >= fat->root_ent_cnt)) // 索引是无效
    {
        return (diritem_t *)0;
    }
    int offset = index * sizeof(diritem_t);                     // 字节偏移
    int sector = fat->root_start + offset / fat->bytes_per_sec; // 目录项在整个分区中的位置
    int err = bread_sector(fat, sector);                        // 读缓存
    if (err < 0)                                                // 读取失败
    {
        return (diritem_t *)0;
    }
    return (diritem_t *)(fat->fat_buf + offset % fat->bytes_per_sec);
}
/**
 * @brief        : 在root目录中写入diritem
 * @param         {fat_t} *fat: fat文件系统描述符指针
 * @param         {diritem_t} *item: 要写入的目录表项
 * @param         {int} index: 索引
 * @return        {int} : 写入成功返回0,否则返回-1
 **/
static int write_dir_entry(fat_t *fat, diritem_t *item, int index)
{
    if ((index < 0) || (index >= fat->root_ent_cnt)) // 索引是无效
    {
        return -1;
    }

    int offset = index * sizeof(diritem_t);                     // 字节偏移
    int sector = fat->root_start + offset / fat->bytes_per_sec; // 目录项在整个分区中的位置
    int err = bread_sector(fat, sector);                        // 读缓存
    if (err < 0)                                                // 读取失败
    {
        return -1;
    }
    kernel_memcpy((void *)(fat->fat_buf + offset % fat->bytes_per_sec), (void *)item, sizeof(diritem_t)); // 写入
    return bwrite_sector(fat, sector);
}
/**
 * @brief        : 对目录表项的初始化
 * @param         {diritem_t *} item: 需要初始化的目录表项
 * @param         {uint8_t} attr: 属性
 * @param         {char *} name: 目录项名称
 * @return        {int} : 返回0
 **/
int diritem_init(diritem_t *item, uint8_t attr, const char *name)
{
    to_sfn((char *)item->DIR_Name, name);
    item->DIR_FstClusHI = (uint16_t)(FAT_CLUSTER_INVALID >> 16);
    item->DIR_FstClusL0 = (uint16_t)(FAT_CLUSTER_INVALID & 0xFFFF);
    item->DIR_FileSize = 0;
    item->DIR_Attr = attr;
    item->DIR_NTRes = 0;

    // 时间写固定值，简单方便
    item->DIR_CrtTime = 0;
    item->DIR_CrtDate = 0;
    item->DIR_WrtTime = item->DIR_CrtTime;
    item->DIR_WrtDate = item->DIR_CrtDate;
    item->DIR_LastAccDate = item->DIR_CrtDate;
    return 0;
}
/**
 * @brief        : 设置当前簇的下一个簇的簇号
 * @param         {fat_t} *fat: fat文件系统描述符指针
 * @param         {cluster_t} curr_cluster: 当前簇号
 * @param         {cluster_t} next_cluster: 下一个簇簇号
 * @return        {int} 设置成功返回0,否则返回-1
 **/
int cluster_set_next(fat_t *fat, cluster_t curr_cluster, cluster_t next_cluster)
{
    if (!cluster_is_valid(curr_cluster))
    {
        return -1;
    }
    int offset = curr_cluster * sizeof(cluster_t);   // 计算当前簇在FAT表中的字节偏移
    int sector = offset / fat->bytes_per_sec;        // 扇区号 计算该偏移在哪个扇区
    int offset_sector = offset % fat->bytes_per_sec; // 计算在扇区内的偏移
    if (sector >= fat->table_sectors)                // 扇区号超出范围
    {
        log_printf("cluster too big:%d\n", curr_cluster);
        return -1;
    }
    int err = bread_sector(fat, fat->table_start + sector); // 将包含目标簇信息的扇区读入 fat->fat_buf 缓冲区
    if (err < 0)
    {
        return -1;
    }
    *(cluster_t *)(fat->fat_buf + offset_sector) = next_cluster;
    for (int i = 0; i < fat->table_cnt; i++)
    {
        int err = bwrite_sector(fat, fat->table_start + sector);
        if (err < 0)
        {
            log_printf("write cluster failed.\n");
            return -1;
        }
        sector += fat->table_sectors;
    }
    return 0;
}
/**
 * @brief        : 获取下一个簇号
 * @param         {fat_t *} fat: fat文件系统描述结构指针
 * @param         {cluster_t} curr_cluster: 当前簇号
 * @return        {int} : 下一个簇号
 **/
int cluster_get_next(fat_t *fat, cluster_t curr_cluster)
{
    if (!cluster_is_valid(curr_cluster))
    {
        return FAT_CLUSTER_INVALID;
    }
    int offset = curr_cluster * sizeof(cluster_t);   // 计算当前簇在FAT表中的字节偏移
    int sector = offset / fat->bytes_per_sec;        // 扇区号 计算该偏移在哪个扇区
    int offset_sector = offset % fat->bytes_per_sec; // 计算在扇区内的偏移
    if (sector >= fat->table_sectors)                // 扇区号超出范围
    {
        log_printf("cluster too big:%d\n", curr_cluster);
        return FAT_CLUSTER_INVALID;
    }
    int err = bread_sector(fat, fat->table_start + sector); // 将包含目标簇信息的扇区读入 fat->fat_buf 缓冲区
    if (err < 0)
    {
        return FAT_CLUSTER_INVALID;
    }
    return *(cluster_t *)(fat->fat_buf + offset_sector);
}
/**
 * @brief        : 释放簇链
 * @param         {fat_t *} fat: fat文件系统结构描述符指针
 * @param         {cluster_t} start: 起始簇号
 * @return        {void}
 **/
void cluster_free_chain(fat_t *fat, cluster_t start)
{
    while (cluster_is_valid(start)) // 当前簇号有效
    {
        cluster_t next = cluster_get_next(fat, start);  // 获取下一个簇
        cluster_set_next(fat, start, FAT_CLUSTER_FREE); // 设置当前簇的下一个簇为空
        start = next;                                   // 设置为下一个
    }
}
/**
 * @brief        : 打开某文件系统下的指定路径的文件
 * @param         {_fs_t} *fs: 指定文件系统的指针
 * @param         {char *} path: 文件的路径
 * @param         {file_t *} file: 需要打开的文件
 * @return        {int} : 成功返回0,失败返回-1
 **/
int fatfs_open(struct _fs_t *fs, const char *path, file_t *file)
{
    fat_t *fat = (fat_t *)fs->data;             // 取出fat文件系统指针
    diritem_t *file_item = (diritem_t *)0;      // 记录文件项
    int f_index = -1;                           // 记录文件表项索引
    for (int i = 0; i < fat->root_ent_cnt; i++) // 遍历根目录的数据区，找到已经存在的匹配项
    {
        diritem_t *item = read_dir_entry(fat, i); // 取目录项
        if (item == (diritem_t *)0)
        {
            return -1;
        }

        if (item->DIR_Name[0] == DIRITEM_NAME_END) // 结束项
        {
            f_index = i;
            break;
        }
        if (item->DIR_Name[0] == DIRITEM_NAME_FREE) // 空闲项
        {
            f_index = i;
            continue;
        }
        // 找到有效表项
        if (diritem_name_match(item, path)) // 路径匹配
        {
            file_item = item;
            f_index = i;
            break;
        }
    }
    if (file_item) // 找到表项
    {
        read_from_diritem(fat, file, file_item, f_index);
        if (file->mode & O_TRUNC)
        {
            cluster_free_chain(fat, file->start_block);                    // 清空簇链
            file->current_block = file->start_block = FAT_CLUSTER_INVALID; // 设置为无效值
            file->size = 0;
        }
        return 0;
    }
    else if ((file->mode & O_CREAT) && (f_index >= 0))
    {                                 // 未找到,创建新表项
        diritem_t item;               // 创建一个空闲的diritem项
        diritem_init(&item, 0, path); // 初始化
        int err = write_dir_entry(fat, &item, f_index);
        if (err < 0)
        {
            log_printf("create file failed.\n");
            return -1;
        }
        read_from_diritem(fat, file, &item, f_index); // 读取数据存入file中
        return 0;
    }
    return -1;
}

/**
 * @brief        : 移动文件访问位置
 * @param         {file_t *} file: 需要操作的文件
 * @param         {fat_t *} fat: fat文件系统描述结构指针
 * @param         {uint32_t} move_bytes: 移动的字节量
 * @param         {int} expand: 是否增减
 * @return        {int} : 成功返回0,失败返回-1
 **/
static int move_file_pos(file_t *file, fat_t *fat, uint32_t move_bytes, int expand)
{
    uint32_t c_offset = file->pos % fat->cluster_byte_size; // 计算簇内偏移
    // 跨簇，则调整curr_cluster。注意，如果已经是最后一个簇了，则curr_cluster不会调整
    if (c_offset + move_bytes >= fat->cluster_byte_size) // 超过簇大小
    {
        cluster_t next_cluster = cluster_get_next(fat, file->current_block); // 获取下一个簇号
        if ((next_cluster == FAT_CLUSTER_INVALID) && expand)                 // 簇结束
        {
            int err = expand_file(file, fat->cluster_byte_size); // 扩充
            if (err < 0)
            {
                return -1;
            }
            next_cluster = cluster_get_next(fat, file->current_block);
        }
        file->current_block = next_cluster; // 指向下一个簇号
    }
    file->pos += move_bytes;
    return 0;
}
/**
 * @brief        : 读取某文件
 * @param         {char} *buf: 读入的缓冲区
 * @param         {int} size: 读取的数据量
 * @param         {file_t} *file: 读取的文件
 * @return        {int} : 实际读取的数据量
 **/
int fatfs_read(char *buf, int size, file_t *file)
{
    fat_t *fat = (fat_t *)file->fs->data; // 获取FAT文件系统描述符指针
    uint32_t nbytes = size;               // size备份
    if (file->pos + nbytes > file->size)  // 大于文件末尾
    {
        nbytes = file->size - file->pos; // 调整读写大小
    }
    uint32_t total_read = 0; // 总读取数据量
    while (nbytes > 0)       // 读取
    {
        uint32_t curr_read = nbytes;                                                                // 记录当前读取数据量
        uint32_t cluster_offset = file->pos % fat->cluster_byte_size;                               // 簇偏移
        uint32_t start_sector = fat->data_start + (file->current_block - 2) * fat->sec_per_cluster; // 起始扇区号
        // 如果是整簇, 只读一簇
        if ((cluster_offset == 0) && (nbytes == fat->cluster_byte_size)) // 在簇起始处且刚好读一个簇
        {
            int err = dev_read(fat->fs->dev_id, start_sector, buf, fat->sec_per_cluster); // 读取数据到buf
            if (err < 0)                                                                  // 读取失败
            {
                return total_read;
            }
            curr_read = fat->cluster_byte_size;
        }
        else // 簇中间
        {
            // 如果跨簇，只读第一个簇内的一部分
            if (cluster_offset + curr_read > fat->cluster_byte_size) // 读取大小超过簇末尾
            {
                curr_read = fat->cluster_byte_size - cluster_offset; // 调整到读取簇末尾
            }
            fat->curr_sector = -1;
            int err = dev_read(fat->fs->dev_id, start_sector, fat->fat_buf, fat->sec_per_cluster); // 读取数据到fat->fat_buf终
            if (err < 0)
            {
                return total_read;
            }
            kernel_memcpy((void *)buf, (void *)(fat->fat_buf + cluster_offset), curr_read); // 复制到fat->fat_buf
        }
        buf += curr_read;
        nbytes -= curr_read;                              // 减去读取的大小
        total_read += curr_read;                          // 读取的数据量增加
        int err = move_file_pos(file, fat, curr_read, 0); // 调整文件读取位置
        if (err < 0)
        {
            return total_read;
        }
    }
    return total_read;
}
/**
 * @brief        : 分配空闲的簇
 * @param         {fat_t} *fat: fat文件系统结构指针
 * @param         {int} cnt: 分配的数量
 * @return        {cluster_t} : 分配的簇的编号
 **/
cluster_t cluster_alloc_free(fat_t *fat, int cnt)
{
    cluster_t pre, curr, start;
    int c_total = fat->table_sectors * fat->bytes_per_sec / sizeof(cluster_t);
    pre = FAT_CLUSTER_INVALID;
    start = FAT_CLUSTER_INVALID;
    for (curr = 2; cnt && (curr < c_total); curr++)
    {
        cluster_t free_cluster = cluster_get_next(fat, curr); // 取当前簇下一个簇
        if (free_cluster == FAT_CLUSTER_FREE)                 // 是否为空闲
        {
            if (!cluster_is_valid(start)) // 为空闲且有效
            {
                start = curr; // 指向第一个空闲表项
            }
            if (cluster_is_valid(pre))
            {
                int err = cluster_set_next(fat, pre, curr);
                if (err < 0)
                {
                    cluster_free_chain(fat, start);
                    return FAT_CLUSTER_INVALID;
                }
            }
            pre = curr;
            cnt--;
        }
    }
    if (cnt == 0)
    {
        int err = cluster_set_next(fat, pre, FAT_CLUSTER_INVALID); // 结束标记
        if (err == 0)
        {
            return start;
        }
    }
    cluster_free_chain(fat, start);
    return FAT_CLUSTER_INVALID;
}

/**
 * @brief        : 扩充文件大小
 * @param         {file_t *} file: 需要扩充的文件的指针
 * @param         {int} incr_bytes: 增加的大小
 * @return        {int} : 扩充成功返回
 **/
static int expand_file(file_t *file, int incr_bytes)
{
    fat_t *fat = (fat_t *)file->fs->data; // 获取fat结构
    int cluster_cnt = 0;
    // 当前没有分配簇或者当前文件所占簇刚好为整数
    if ((file->size == 0) || (file->size % fat->cluster_byte_size == 0))
    {
        cluster_cnt = up2(incr_bytes, fat->cluster_byte_size) / fat->cluster_byte_size; // 向上取整
    }
    else
    {
        int cfree = fat->cluster_byte_size - (file->size % fat->cluster_byte_size); // 当前簇剩余容量
        if (cfree > incr_bytes)                                                     // 当前簇空闲大于需要扩充的
        {
            return 0;
        }
        cluster_cnt = up2(incr_bytes - cfree, fat->cluster_byte_size) / fat->cluster_byte_size; // 需要分配的簇的数量
        if (cluster_cnt == 0)
        {
            cluster_cnt = 1;
        }
    }
    cluster_t start_cluster = cluster_alloc_free(fat, cluster_cnt); // 分配空闲的簇
    if (!cluster_is_valid(start_cluster))
    {
        log_printf("no valid cluster for file write.\n");
        return -1;
    }
    if (!cluster_is_valid(file->start_block)) // 文件未分配簇
    {

        file->start_block = file->current_block = start_cluster; // 设置当前簇号与起始簇号
    }
    else
    {
        int err = cluster_set_next(fat, file->current_block, start_cluster); // 设置当前簇号,建立链接关系
        if (err < 0)
        {
            log_printf("write cluster failed.\n");
            return -1;
        }
    }
    return 0;
}
/**
 * @brief        : 写文件
 * @param         {char} *buf: 写入的数据的指针
 * @param         {int} size: 写入的数据量
 * @param         {file_t *} file: 写入的文件
 * @return        {int} : 实际写入的数据量
 **/
int fatfs_write(char *buf, int size, file_t *file)
{
    fat_t *fat = file->fs->data;       // 获取fat结构
    if (file->pos + size > file->size) // 超过文件大小
    {
        int incr_size = file->pos + size - file->size; // 需要扩充的容量
        int err = expand_file(file, incr_size);        // 扩充
        if (err < 0)
        {
            log_printf("expand failed.\n");
            return 0;
        }
    }
    uint32_t nbytes = size;
    uint32_t total_write = 0;
    while (nbytes)
    {
        uint32_t curr_write = nbytes;                                                               // 记录当前写入数据量
        uint32_t cluster_offset = file->pos % fat->cluster_byte_size;                               // 簇偏移
        uint32_t start_sector = fat->data_start + (file->current_block - 2) * fat->sec_per_cluster; // 起始扇区号
        // 如果是整簇, 只写一簇
        if ((cluster_offset == 0) && (nbytes == fat->cluster_byte_size)) // 在簇起始处且刚好读一个簇
        {
            int err = dev_write(fat->fs->dev_id, start_sector, buf, fat->sec_per_cluster); // 写入数据到
            if (err < 0)                                                                   // 读取失败
            {
                return total_write;
            }
            curr_write = fat->cluster_byte_size;
        }
        else // 簇中间
        {
            // 如果跨簇，只读第一个簇内的一部分
            if (cluster_offset + curr_write > fat->cluster_byte_size) // 读取大小超过簇末尾
            {
                curr_write = fat->cluster_byte_size - cluster_offset; // 调整到读取簇末尾
            }
            fat->curr_sector = -1;
            int err = dev_read(fat->fs->dev_id, start_sector, fat->fat_buf, fat->sec_per_cluster); // 读取数据到fat->fat_buf终
            if (err < 0)
            {
                return total_write;
            }
            kernel_memcpy((void *)(fat->fat_buf + cluster_offset), (void *)buf, curr_write); // 复制到fat->fat_buf
            err = dev_write(fat->fs->dev_id, start_sector, fat->fat_buf, fat->sec_per_cluster);
            if (err < 0)
            {
                return total_write;
            }
        }
        buf += curr_write;
        nbytes -= curr_write;      // 减去读取的大小
        total_write += curr_write; // 读取的数据量增加
        file->size += curr_write;
        int err = move_file_pos(file, fat, curr_write, 1); // 调整文件读取位置
        if (err < 0)
        {
            return total_write;
        }
    }
    return total_write;
}
/**
 * @brief        : 关闭指定的文件
 * @param         {file_t *} file: 需要关闭的文件
 * @return        {void}
 **/
void fatfs_close(file_t *file)
{
    if (file->mode == O_RDONLY) // 只读
    {
        return;
    }
    fat_t *fat = (fat_t *)file->fs->data;                 // 获取fat结构
    diritem_t *item = read_dir_entry(fat, file->f_index); // 读取出当前的目录表项
    if (item == (diritem_t *)0)                           // 为空
    {
        return;
    }
    item->DIR_FileSize = file->size;                              // 设置目录表项大小
    item->DIR_FstClusHI = (uint16_t)(file->start_block >> 16);    // 设置高16位
    item->DIR_FstClusL0 = (uint16_t)(file->start_block & 0xFFFF); // 设置低16位
    write_dir_entry(fat, item, file->f_index);                    // 回写
    return;
}
/**
 * @brief        : 跳转到文件指定位置操作
 * @param         {file_t *} file: 需要操作的文件
 * @param         {uint32_t} offset: 偏移量
 * @param         {int} dir:  操作的起始位置
 * @return        {int} : 指定位置的指针
 **/
int fatfs_seek(file_t *file, uint32_t offset, int dir)
{
    if (dir != 0) // 不支持此种操作
    {
        return -1;
    }
    fat_t *fat = file->fs->data;                   // 取fat结构
    cluster_t current_cluster = file->start_block; // 应该从 start_block 开始，不是 current_block
    uint32_t curr_pos = 0;                         // 记录当前的位置
    uint32_t offset_to_move = offset;              // 需要移动的偏移量
    while (offset_to_move)
    {
        uint32_t curr_offest = curr_pos % fat->cluster_byte_size; // 当前簇内偏移
        uint32_t curr_move = offset_to_move;                      // 准备移动的字节数
        if (curr_offest + curr_move < fat->cluster_byte_size)     // 如果在当前簇内就能完成移动
        {
            curr_pos += curr_move;
            break;
        }
        // 需要跨簇移动
        curr_move = fat->cluster_byte_size - curr_offest;
        curr_pos += curr_move;
        offset_to_move -= curr_move;
        // 移动到下一个簇
        current_cluster = cluster_get_next(fat, current_cluster);
        if (!cluster_is_valid(current_cluster))
        {
            return -1;
        }
    }
    file->pos = curr_pos;                  // 更新文件位置
    file->current_block = current_cluster; // 更新当前簇
    return 0;
}
/**
 * @brief        : 获取文件状态信息
 * @param         {file_t} *file: 需要获取状态信息的文件对象
 * @param         {stat *} st: 用于存储文件状态信息的结构体指针
 * @return        {int} : 成功返回0，失败返回负数错误码
 **/
int fatfs_stat(file_t *file, struct stat *st)
{
    return 0;
}
/**
 * @brief        : 打开目录
 * @param         {struct _fs_t} *fs: 文件系统对象指针
 * @param         {const char} *name: 要打开的目录名称
 * @param         {DIR} *dir: 用于存储目录信息的DIR结构体指针
 * @return        {int} : 成功返回0，失败返回负数错误码
 **/
int fatfs_opendir(struct _fs_t *fs, const char *name, DIR *dir)
{
    dir->index = 0; // 取第0项
    return 0;
}
/**
 * @brief        : 获取目录项名称
 * @param         {diritem_t*} item: 需要读取得目录项
 * @param         {char *} dest: 读取到得目的地址
 * @return        {void}
 **/
void diritem_get_name(diritem_t *item, char *dest)
{
    char *c = dest;             // 保存读入目的地址
    kernel_memset(dest, 0, SFN_LEN + 1); // 清空目的地址存储内容
    char *ext = (char *)0;      // 记录扩展名位置
    for (int i = 0; i < 11; i++)
    {
        if (item->DIR_Name[i] != ' ') // 不为空
        {
            *c++ = item->DIR_Name[i]; // 取出并保存
        }
        if (i == 7) // 扩展名前一个字符
        {
            ext = c;    // 记录扩展名位置
            *c++ = '.'; // .
        }
    }
    if (ext && (ext[1] == '\0')) // 没有扩展名直接结束
    {
        ext[0] = '\0';
    }
}
/**
 * @brief        : 读取目录项
 * @param         {struct _fs_t} *fs: 文件系统对象指针
 * @param         {DIR} *dir: 已打开的目录对象指针
 * @param         {struct dirent} *dirent: 用于存储目录项信息的结构体指针
 * @return        {int} : 成功返回0，失败返回负数错误码
 **/
int fatfs_readdir(struct _fs_t *fs, DIR *dir, struct dirent *dirent)
{
    fat_t *fat = (fat_t *)fs->data; // 修改为 fs->data
    while (dir->index < fat->root_ent_cnt) // 扫描根目录
    {
        diritem_t *item = read_dir_entry(fat, dir->index); // 读取目录项
        if ((diritem_t *)0 == item)                        // 读取失败
        {
            return -1;
        }
        if (item->DIR_Name[0] == DIRITEM_NAME_END) // 结束标志
        {
            break;
        }

        // 只处理非空闲项
        if (item->DIR_Name[0] != DIRITEM_NAME_FREE) // 修改条件
        {
            file_type_t type = diritem_get_type(item);       // 获取类型
            if ((type == FILE_NORMAL) || (type == FILE_DIR)) // 修复逻辑错误
            {
                dirent->index = dir->index++; // 移动到正确位置
                dirent->type = type;          // 使用已获取的type
                dirent->size = item->DIR_FileSize;
                diritem_get_name(item, dirent->name);
                return 0;
            }
        }
        dir->index++; // 在循环末尾递增索引
    }
    return -1;
}
/**
 * @brief        : 关闭目录
 * @param         {struct _fs_t} *fs: 文件系统对象指针
 * @param         {DIR} *dir: 要关闭的目录对象指针
 * @return        {int} : 成功返回0，失败返回负数错误码
 **/
int fatfs_closedir(struct _fs_t *fs, DIR *dir)
{
    return 0;
}

/**
 * @brief        : 从磁盘中删除文件
 * @param         {_fs_t} *fs: 文件系统描述符的指针
 * @param         {char *} pathname: 路径名称
 * @return        {int} : 成功返回0,失败返回-1
 **/
int fatfs_unlink(struct _fs_t *fs, const char *pathname)
{
    fat_t *fat = (fat_t *)fs->data;             // 取出fat文件系统指针
    for (int i = 0; i < fat->root_ent_cnt; i++) // 遍历根目录的数据区，找到已经存在的匹配项
    {
        diritem_t *item = read_dir_entry(fat, i); // 取目录项
        if (item == (diritem_t *)0)
        {
            return -1;
        }

        if (item->DIR_Name[0] == DIRITEM_NAME_END) // 结束项
        {
            break;
        }
        if (item->DIR_Name[0] == DIRITEM_NAME_FREE) // 空闲项
        {
            continue;
        }
        // 找到有效表项
        if (diritem_name_match(item, pathname)) // 路径匹配,执行删除操作
        {
            int start_cluster = (item->DIR_FstClusHI << 16) | item->DIR_FstClusL0; // 起始簇号
            cluster_free_chain(fat, start_cluster);                                // 释放簇链

            diritem_t new_item;
            kernel_memset(&new_item, 0, sizeof(diritem_t));
            return write_dir_entry(fat, &new_item, i);
        }
    }
    return -1; // 没有找到
}
fs_op_t fatfs_op = {
    .mount = fatfs_mount,
    .unmount = fatfs_unmount,
    .open = fatfs_open,
    .read = fatfs_read,
    .write = fatfs_write,
    .close = fatfs_close,
    .seek = fatfs_seek,
    .stat = fatfs_stat,
    .opendir = fatfs_opendir,
    .readdir = fatfs_readdir,
    .closedir = fatfs_closedir,
    .unlink = fatfs_unlink,
};
