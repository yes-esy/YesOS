/**
 * @FilePath     : /code/source/kernel/fs/fs.c
 * @Description  :  文件系统实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-11 20:13:25
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "fs/fs.h"
#include "tools/klib.h"
#include "comm/boot_info.h"
#include "comm/cpu_instr.h"
#include "tools/log.h"
#include "dev/console.h"
#include "fs/file.h"
#include "dev/dev.h"
#include "core/task.h"
#include "cpu/irq.h"
#include <sys/file.h>
#include "dev/disk.h"
#include "os_cfg.h"
// #define TEMP_FILE_ID 100
// #define TEMP_ADDR (8 * 1024 * 1024)  // 在0x800000处缓存原始
// static uint8_t *temp_pos;            // 文件读写位置
static list_t mounted_list;          // 已挂载的文件系统的链表
static fs_t fs_table[FS_TABLE_SIZE]; // 文件系统数组
static list_t free_list;             // 空闲的的结点的文件系统链表
extern fs_op_t devfs_op;             // 外部设备文件系统
extern fs_op_t fatfs_op;             // 外部fat文件系统
static fs_t *root_fs;                // 根文件系统
/**
 * @brief        : 使用LBA48模式读取磁盘
 * @param         {uint32_t} sector: 扇区号
 * @param         {uint32_t} sector_count: 扇区数量
 * @param         {uint8_t *} buff: 输入缓冲区
 * @return        {*} 程序入口地址
 **/
static void read_disk(uint32_t sector, uint32_t sector_count, uint8_t *buf)
{
    irq_state_t irq_state = irq_enter_protection();
    outb(0x1F6, (uint8_t)(0xE0));

    outb(0x1F2, (uint8_t)(sector_count >> 8));
    outb(0x1F3, (uint8_t)(sector >> 24)); // LBA参数的24 ~ 31位
    outb(0x1F4, (uint8_t)(0));            // LBA参数的32~39位
    outb(0x1F5, (uint8_t)(0));            // LBA参数的40~47位

    outb(0x1F2, (uint8_t)(sector_count));
    outb(0x1F3, (uint8_t)(sector));       // LBA参数的0~7位
    outb(0x1F4, (uint8_t)(sector >> 8));  // LBA参数的8~15位
    outb(0x1F5, (uint8_t)(sector >> 16)); // LBA参数的16~23位

    outb(0x1F7, (uint8_t)0x24);

    // 读取数据
    uint16_t *data_buf = (uint16_t *)buf;
    while (sector_count-- > 0)
    {
        // 每次扇区读之前都要检查,等待数据就绪
        while ((inb(0x1F7) & 0x88) != 0x8)
        {
        }

        // 读取数据并将数据写入到缓存中
        for (int i = 0; i < SECTOR_SIZE / 2; i++)
        {
            *data_buf++ = inw(0x1F0);
        }
    }
    irq_leave_protection(irq_state);
}
/**
 * @brief        : 判断文件路径是否有效
 * @param         {char *} path: 文件路径指针
 * @return        {int} 若有效返回1,无效返回0
 **/
static int is_path_valid(const char *path)
{
    if ((path == (const char *)0) || (path[0] == '\0')) // 路径为空
    {
        return 0; // 无效
    }
    return 1; // 有效
}
/**
 * @brief        : 判断文件表索引是否合法
 * @param         {int} file: 需要判断的文件在文件表中的索引
 * @return        {int} : 返回1为不合法,0为合法
 **/
static int is_fd_bad(int file)
{
    if ((file < 0) || (file >= TASK_OPEN_FILE_NR))
    {
        return 1;
    }
    return 0;
}
/**
 * @brief        : 路径转数字
 * @param         {char *} path: 路径
 * @param         {int *} num: 转成数字存储的变量
 * @return        {int} : 成功返回0,失败返回-1
 **/
int path_to_num(const char *path, int *num)
{
    int n = 0;
    const char *c = path;
    while (*c)
    {
        n = n * 10 + (*c - '0');
        c++;
    }
    *num = n;
    return 0;
}
/**
 * @brief        : 获取下一个字路径
 * @param         {char *} path: 传入的路径
 * @return        {const char *} : 下一个路径开始的指针
 **/
const char *path_next_child(const char *path)
{
    const char *c = path;
    while ((*c) && (*c++ == '/')) // 跳过'/'
        ;
    while ((*c) && (*c++ != '/')) // 跳到 '/'后面一个字符
        ;
    return *c ? c : (const char *)0;
}
/**
 * @brief        : 判断某路径是否以指定形式开头
 * @param         {char *} path: 需要判断的路径
 * @param         {char *} str: 指定的开头
 * @return        {int} 相同返回0,否则返回-1
 **/
int path_begin_with(const char *path, const char *str)
{
    const char *s1 = path, *s2 = str;
    while (*s1 && *s2 && (*s1 == *s2)) // 是否相同
    {
        s1++;
        s2++;
    }
    return *s2 == '\0'; //  匹配到s2的末尾则说明相同
}
/**
 * @brief        : 对文件系统上锁
 * @param         {fs_t *} fs: 文件系统描述符指针
 * @return        {void}
 **/
static void fs_protect(fs_t *fs)
{
    if (fs->mutex) // 是否有效
    {
        mutex_lock(fs->mutex); // 上锁
    }
}
/**
 * @brief        : 对文件系统解锁
 * @param         {fs_t *} fs: 文件系统描述符指针
 * @return        {void}
 **/
static void fs_unprotect(fs_t *fs)
{
    if (fs->mutex) // 是否有效
    {
        mutex_unlock(fs->mutex); // 上锁
    }
}
/**
 * @brief        : 打开文件
 * @param         {char} *path: 文件路径
 * @param         {int} flags: 标志
 * @return        {int} : 若成功返回文件描述符,失败返回-1
 **/
int sys_open(const char *path, int flags, ...)
{
    // // 判断是否为shell.elf
    // if (kernel_strncmp(path, "/shell.elf", 4) == 0)
    // {
    //     // log_printf("Opening file: %s\n", path);
    //     // 暂时直接从扇区5000上读取, 读取大概40KB，足够了
    //     // read_disk(5000, 80, (uint8_t *)TEMP_ADDR);
    //     int dev_id = dev_open(DEV_DISK, 0xA0, (void *)0);
    //     dev_read(dev_id, 5000, (uint8_t *)TEMP_ADDR, 80);
    //     temp_pos = (uint8_t *)TEMP_ADDR;
    //     return TEMP_FILE_ID;
    // }
    if (!is_path_valid(path)) // 路径是否有效
    {
        log_printf("path(%s) is not valid.\n", path);
        return -1;
    }
    int fd = -1;                 // 记录文件描述符在进程文件表中对应的索引
    file_t *file = file_alloc(); // 分配一个文件描述符
    if (file == (file_t *)0)     // 分配失败
    {
        return -1;
    }
    fd = task_alloc_fd(file); // 添加该文件到进程文件表中
    if (fd < 0)               // 添加失败
    {
        goto sys_open_failed; // 错误处理
    }
    fs_t *fs = (fs_t *)0;                          // 记录对应的文件系统的指针
    list_node_t *node = list_first(&mounted_list); // 取出挂载的第一个文件系统的结点
    while (node)
    {
        fs_t *curr = list_node_parent(node, fs_t, node);
        if (path_begin_with(path, curr->mount_point)) // 找到了对应的文件系统
        {
            fs = curr; // 记录
            break;
        }
        node = list_node_next(node); // 下一个结点
    }
    const char *name = (const char *)0;
    if (fs) // 找到了
    {
        name = path_next_child(path); // 下一个路径
    }
    else // 没有找到
    {
        fs = root_fs; // 根文件系统作为默认文件系统
        name = path;
    }
    // 相关字段初始化
    file->mode = flags;                                                   // 打开模式
    file->fs = fs;                                                        // 记录文件系统
    kernel_memcpy((void *)file->file_name, (void *)name, FILE_NAME_SIZE); // 设置文件名
    fs_protect(fs);                                                       // 进入保护
    int err = fs->op->open(fs, name, file);                               // 通过文件系统打开该文件
    if (err < 0)                                                          // 打开失败
    {
        fs_unprotect(fs); // 退出保护
        log_printf("open %s file system failed.\n", name);
        goto sys_open_failed;
    }
    fs_unprotect(fs); // 退出保护
    return fd;        // 返回文件描述符在进程文件表中对应的索引
sys_open_failed:
    file_free(file); // 释放文件
    if (fd >= 0)
    {
        task_remove_fd(fd); // 从进程文件表释放该文件
    }
    return -1;
}
/**
 * @brief        : 读取文件
 * @param         {int} file: 哪一个文件
 * @param         {char} *ptr: 读取到文件的目的地址
 * @param         {int} len: 读取长度
 * @return        {int} 读取成功返回读取长度,失败返回-1
 **/
int sys_read(int file, char *ptr, int len)
{
    // if (file == TEMP_FILE_ID) // 读取shell.elf
    // {
    //     kernel_memcpy(ptr, temp_pos, len);
    //     temp_pos += len;
    //     return len;
    // }
    if (is_fd_bad(file) || !ptr || !len) // 参数不合法
    {
        log_printf("read param valid.\n");
        return -1;
    }

    file_t *cur_file = task_file(file); // 获取当前文件在进程中的文件描述符
    if (cur_file == (file_t *)0)        // 获取为空
    {
        log_printf("file not opened , file is %d\n", file);
        return -1;
    }
    if (cur_file->mode == O_WRONLY) // 只写的方式
    {
        log_printf("file(%s) not allowed read\n", cur_file->file_name);
        return -1;
    }
    fs_t *fs = cur_file->fs;                         // 获取当前文件系统描述符
    fs_protect(fs);                                  // 进入保护
    int read_len = fs->op->read(ptr, len, cur_file); // 读取操作
    fs_unprotect(fs);                                // 退出保护
    return read_len;                                 // 返回读取长度
}
/**
 * @brief        : 写文件
 * @param         {int} file: 写入的文件
 * @param         {char} *ptr: 写入的内容的起始地址
 * @param         {int} len: 写入长度
 * @return        {int} : 若成功返回写入长度,失败返回-1
 **/
int sys_write(int file, char *ptr, int len)
{
    // if (file == 1)
    // {
    //     // ptr[len] = '\0';
    //     // log_printf("%s", ptr);
    //     // console_write(0, ptr, len);
    //     char buf[128];
    //     kernel_memcpy(buf, ptr, len);
    //     buf[len] = '\0';
    //     log_printf("%s", buf);
    // }
    // file = 0;
    if (is_fd_bad(file) || !ptr || !len) // 参数不合法
    {
        log_printf("write param valid.\n");
        return -1;
    }
    file_t *cur_file = task_file(file); // 获取当前文件在进程中的文件描述符
    if (cur_file == (file_t *)0)        // 获取为空
    {
        log_printf("file not opened , file is %d\n", file);
        return -1;
    }
    if (cur_file->mode == O_RDONLY) // 只读的方式,不允许写
    {
        log_printf("file(%s) not allowed write\n", cur_file->file_name);
        return -1;
    }
    fs_t *fs = cur_file->fs;                           // 获取当前文件系统描述符
    fs_protect(fs);                                    // 进入保护
    int write_len = fs->op->write(ptr, len, cur_file); // 写入操作
    fs_unprotect(fs);                                  // 退出保护
    return write_len;                                  // 返回写入长度
}
/**
 * @brief        : 调整读写指针
 * @param         {int} file: 操作文件
 * @param         {int} ptr: 要调整的指针
 * @param         {int} dir: 文件所在目录
 * @return        {int} 成功返回 0, 失败返回 -1
 **/
int sys_lseek(int file, int ptr, int dir)
{
    // if (file == TEMP_FILE_ID)
    // {
    //     temp_pos = (uint8_t *)(TEMP_ADDR + ptr); // 调整指针
    //     return 0;
    // }
    if (is_fd_bad(file)) // 参数不合法
    {
        return -1;
    }
    file_t *cur_file = task_file(file); // 获取当前文件在进程中的文件描述符
    if (cur_file == (file_t *)0)        // 获取为空
    {
        log_printf("file not opened , file is %d\n", file);
        return -1;
    }
    fs_t *fs = cur_file->fs;                         // 获取当前文件系统描述符
    fs_protect(fs);                                  // 进入保护
    int seek_ptr = fs->op->seek(cur_file, ptr, dir); // 调整读写指针操作
    fs_unprotect(fs);                                // 退出保护
    return seek_ptr;
}
/**
 * @brief        : 关闭某个打开的文件
 * @param         {int} file: 操作的文件
 * @return        {int} : 成功关闭返回0,失败返回-1
 **/
int sys_close(int file)
{
    // if (file == TEMP_FILE_ID)
    // {
    //     return 0;
    // }
    if (is_fd_bad(file)) // 参数不合法
    {
        log_printf("close param valid\n");
        return -1;
    }
    file_t *curr_file = task_file(file); // 获取当前文件
    if (curr_file == (file_t *)0)
    {
        log_printf("get file error.\n");
        return -1;
    }
    ASSERT(curr_file->ref > 0); // 文件一定是被打开过的
    if (curr_file->ref == 1)    // 只打开了一次,才需要关闭文件,否则增加引用次数即可
    {
        fs_t *fs = curr_file->fs; // 获取文件系统描述符指针
        fs_protect(fs);           // 进入保护
        fs->op->close(curr_file); // 调用文件系统关闭操作
        fs_unprotect(fs);         // 退出保护
        file_free(curr_file);     // 释放当前文件结构描述符
    }
    task_remove_fd(file); // 在进程的文件表中释放该文件
    return 0;             // 成功关闭
}
/**
 * @brief        : 判断文件是否为tty设备
 * @param         {int} file: 操作的文件
 * @return        {int} : 返回0表示不为tty设备,否则为tty设备
 **/
int sys_isatty(int file)
{
    if (is_fd_bad(file)) // 参数不合法
    {
        log_printf("isatty param valid\n");
        return 0;
    }
    file_t *curr_file = task_file(file); // 获取当前文件
    if (curr_file == (file_t *)0)
    {
        log_printf("get file error.\n");
        return 0;
    }
    return curr_file->type == FILE_TTY;
}
/**
 * @brief        : 获取文件状态信息
 * @param         {int} file: 文件描述符
 * @param         {struct stat} *st: 存储文件信息的结构体指针
 * @return        {int} 成功返回0，失败返回-1
 **/
int sys_fstat(int file, struct stat *st)
{
    if (is_fd_bad(file)) // 参数不合法
    {
        log_printf("close param valid\n");
        return -1;
    }
    file_t *curr_file = task_file(file); // 获取当前文件
    if (curr_file == (file_t *)0)
    {
        log_printf("get file error.\n");
        return -1;
    }
    fs_t *fs = curr_file->fs;                          // 获取文件系统描述符
    kernel_memset((void *)st, 0, sizeof(struct stat)); // 清空stat
    fs_protect(fs);                                    // 进入保护状态
    int err = fs->op->stat(curr_file, st);             // 调用文件系统获取文件信息接口
    fs_unprotect(fs);                                  // 退出保护状态
    return err;
}
/**
 * @brief        : 复制文件描述符
 * @param         {int} file: 要复制的文件描述符索引
 * @return        {int} 新的文件描述符，失败返回-1
 **/
int sys_dup(int file)
{
    if (is_fd_bad(file)) // 文件描述符索引是否合法
    {
        log_printf("file %d id not valid.\n", file);
        return -1;
    }

    file_t *cur_file = task_file(file); // 当前文件描述符
    if (cur_file == (file_t *)0)        // 获取失败
    {
        log_printf("file not opened.\n");
        return -1;
    }
    int fd = task_alloc_fd(cur_file); // 分配一个新表项
    if (fd < 0)                       // 分配失败
    {
        log_printf("no task file available.\n");
        return -1;
    }
    file_incr_ref(cur_file); // 增加打开次数
    return fd;
}
/**
 * @brief        : 打开目录的系统调用接口
 * @param         {const char} *path: 要打开的目录路径
 * @param         {DIR} *dir: 用于存储目录信息的DIR结构体指针
 * @return        {int} : 成功返回0，失败返回负数错误码
 **/
int sys_opendir(const char *path, DIR *dir)
{
    fs_protect(root_fs);                                // 进入保护状态
    int err = root_fs->op->opendir(root_fs, path, dir); // 调用文件系统获取文件信息接口
    fs_unprotect(root_fs);
    return err;
}
/**
 * @brief        : 读取目录项的系统调用接口
 * @param         {DIR} *dir: 已打开的目录对象指针
 * @param         {struct dirent} *dirent: 用于存储目录项信息的结构体指针
 * @return        {int} : 成功返回0，失败返回负数错误码
 **/
int sys_readdir(DIR *dir, struct dirent *dirent)
{
    fs_protect(root_fs);                                  // 进入保护状态
    int err = root_fs->op->readdir(root_fs, dir, dirent); // 调用文件系统获取文件信息接口
    fs_unprotect(root_fs);
    return err;
}
/**
 * @brief        : 关闭目录的系统调用接口
 * @param         {DIR} *dir: 要关闭的目录对象指针
 * @return        {int} : 成功返回0，失败返回负数错误码
 **/
int sys_closedir(DIR *dir)
{
    fs_protect(root_fs);                           // 进入保护状态
    int err = root_fs->op->closedir(root_fs, dir); // 调用文件系统获取文件信息接口
    fs_unprotect(root_fs);
    return err;
}
/**
 * @brief        : 根据文件系统类型获取回调函数表
 * @param         {fs_type_t} type: 文件系统类型
 * @param         {int} major: 文件系统设备号
 * @return        {fs_op_t *}: 回调函数表
 **/
static fs_op_t *get_fs_op(fs_type_t type, int major)
{
    switch (type)
    {
    case FS_DEVFS: // 设备文件系统
        return &(devfs_op);
        break;
    case FS_FAT16: // fat16文件系统
        return &(fatfs_op);
        break;
    default:
        return (fs_op_t *)0;
        break;
    }
    return (fs_op_t *)0;
}
/**
 * @brief        : 挂载文件系统
 * @param         {fs_type_t} type: 文件系统的类型
 * @param         {char *} mount_point: 挂载点
 * @param         {int} dev_major: 主设备号
 * @param         {int} dev_minor: 次设备号
 * @return        {fs_t *} : 挂载成功的文件系统的指针
 **/
static fs_t *mount(fs_type_t type, char *mount_point, int dev_major, int dev_minor)
{
    fs_t *fs = (fs_t *)0; // 记录挂载的文件指针
    log_printf("mounting %s file system, dev:%x\n", mount_point, dev_major);
    list_node_t *curr = list_first(&mounted_list);
    while (curr) // 检查是否已挂载
    {
        fs_t *fs = list_node_parent(curr, fs_t, node);

        if (kernel_strncmp(fs->mount_point, mount_point, FS_MOUNT_POINT_SIZE) == 0) // 重复挂载,挂载失败
        {
            log_printf("%s already mounted\n");
            goto mount_failed;
        }
        curr = list_node_next(curr);
    }
    list_node_t *free_node = list_remove_first(&free_list); // 取出一个空闲结点
    if (!free_node)
    {
        log_printf("no free fs , mount failed\n");
        goto mount_failed;
    }
    fs = list_node_parent(free_node, fs_t, node); // 取出该空闲结点的指针

    fs_op_t *op = get_fs_op(type, dev_major); // 获取函数回调表指针
    if (op == (fs_op_t *)0)
    {
        log_printf("unsupported file system type , type is %s\n", type);
        goto mount_failed;
    }
    kernel_memset(fs, 0, sizeof(fs_t));                                               // 先清空
    kernel_memcpy((void *)fs->mount_point, (void *)mount_point, FS_MOUNT_POINT_SIZE); // 设置挂载点名称
    fs->op = op;                                                                      // 设备函数回调表
    if (op->mount(fs, dev_major, dev_minor) < 0)                                      // 调用挂载函数挂载该文件系统
    {
        // 挂载失败
        log_printf("mount file system failed, mount point is %s\n", mount_point);
        goto mount_failed;
    }
    list_insert_last(&mounted_list, &fs->node); // 插入挂载队列
    return fs;
mount_failed:
    if (fs)
    {
        list_insert_last(&free_list, &fs->node); // 插回原空闲链表
    }
    return (fs_t *)0;
}
/**
 * @brief        : 初始化空闲文件系统链表
 * @return        {*}
 **/
static void mount_list_init(void)
{
    list_init(&free_list);                  // 初始化空闲链表
    for (int i = 0; i < FS_TABLE_SIZE; i++) // 将文件系统表插入空闲链表
    {
        list_insert_first(&free_list, &fs_table[i].node);
    }
    list_init(&mounted_list); // 初始化挂载链表
}
/**
 * @brief        : 输入输出控制
 * @param         {int} file: 输入输出文件
 * @param         {int} cmd: 输入输出命令
 * @param         {int} arg0: 参数0
 * @param         {int} arg1: 参数1
 * @return        {void}
 **/
void sys_ioctl(int file, int cmd, int arg0, int arg1)
{
    if (is_fd_bad(file)) // 参数不合法
    {
        log_printf("isatty param valid\n");
        return;
    }
    file_t *curr_file = task_file(file); // 获取当前文件
    if (curr_file == (file_t *)0)
    {
        log_printf("get file error.\n");
        return;
    }
    fs_t *fs = curr_file->fs; // 获取文件系统描述符指针
    fs_protect(fs);
    int err = fs->op->ioctl(curr_file, cmd, arg0, arg1);
    if (err < 0)
    {
        log_printf("ioctl execute failed.\n");
    }
    fs_unprotect(fs);
}

int sys_unlink(const char *pathname)
{
    fs_protect(root_fs);                     // 进入保护
    int err = root_fs->op->unlink(root_fs,pathname); // 调用删除
    if (err < 0)                             // 删除失败
    {
        fs_unprotect(root_fs);
        return -1;
    }
    fs_unprotect(root_fs); // 退出保护
    return 0;              // 删除成功
}
/**
 * @brief        : 文件系统初始化
 * @return        {void}
 **/
void fs_init(void)
{
    mount_list_init();
    file_table_init();                        // 文件表初始化
    disk_init();                              // 初始化磁盘
    fs_t *fs = mount(FS_DEVFS, "/dev", 0, 0); // 挂载设备文件系统
    ASSERT(fs != (fs_t *)0);

    root_fs = mount(FS_FAT16, "/home", ROOT_DEV); // 挂载FAT16文件系统
    ASSERT(root_fs != (fs_t *)0);
}
