/**
 * @FilePath     : /code/source/kernel/fs/devfs/devfs.c
 * @Description  :  设备文件系统.c头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-09 21:20:34
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "fs/devfs/devfs.h"
#include "fs/fs.h"
#include "dev/dev.h"
#include "tools/klib.h"
#include "tools/log.h"
/**
 * 支持的文件系统的类型
 */
static devfs_type_t devfs_type_list[] = {
    {
        .name = "tty",
        .dev_type = DEV_TTY,
        .file_type = FILE_TTY,
    }, // tty设备
};
/**
 * @brief        : 挂载操作,进行某些初始化
 * @param         {_fs_t} *fs: 指定的文件系统的指针
 * @param         {int} major: 主设备号
 * @param         {int} minor: 次设备号
 * @return        {int} : 成功返回0,失败返回-1
 **/
int devfs_mount(struct _fs_t *fs, int major, int minor)
{
    fs->type = FS_DEVFS;
    return 0;
}
/**
 * @brief        : 取消挂载
 * @param         {_fs_t} *fs: 指定的文件系统的指针
 * @return        {void}
 **/
void devfs_unmont(struct _fs_t *fs)
{
    return;
}
/**
 * @brief        : 打开某文件系统下的指定路径的文件
 * @param         {_fs_t} *fs: 指定文件系统的指针
 * @param         {char *} path: 文件的路径
 * @param         {file_t *} file: 需要打开的文件
 * @return        {int} : 成功返回0,失败返回-1
 **/
int devfs_open(struct _fs_t *fs, const char *path, file_t *file)
{
    // 遍历设备类型表找到相关类型
    for (int i = 0; i < sizeof(devfs_type_list) / sizeof(devfs_type_list[0]); i++)
    {
        devfs_type_t *dev_type = devfs_type_list + i;      // 遍历到的设备
        int type_name_len = kernel_strlen(dev_type->name); // 类型名称长度

        if (kernel_strncmp(path, dev_type->name, type_name_len) == 0) // 找到了某一特定类型的设备
        {
            int minor = 0;                                                                            // 记录次设备号
            if ((kernel_strlen(path) > type_name_len) && (path_to_num(path + type_name_len, &minor))) // 获取次设备号
            {
                log_printf("get device num failed , %s \n", path); // 打开失败
                break;
            }
            int dev_id = dev_open(dev_type->dev_type, minor, (void *)0); // 打开该设备
            if (dev_id < 0) // 打开失败
            {
                log_printf("open device failed.\n");
                break;
            }
            // 初始化操作
            file->dev_id = dev_id;
            file->fs = fs;
            file->pos = 0;
            file->size = 0;
            file->type = dev_type->file_type;
            return 0;
        }
    }
    return -1;
}
/**
 * @brief        : 读取某文件
 * @param         {char} *buf: 读入的缓冲区
 * @param         {int} size: 读取的数据量
 * @param         {file_t} *file: 读取的文件
 * @return        {int} : 实际读取的数据量
 **/
int devfs_read(char *buf, int size, file_t *file)
{
    return dev_read(file->dev_id, file->pos, buf, size);
}
/**
 * @brief        : 写文件
 * @param         {char} *buf: 写入的数据的指针
 * @param         {int} size: 写入的数据量
 * @param         {file_t *} file: 写入的文件
 * @return        {int} : 实际写入的数据量
 **/
int devfs_write(char *buf, int size, file_t *file)
{
    return dev_write(file->dev_id, file->pos, buf, size);
}
/**
 * @brief        : 关闭指定的文件
 * @param         {file_t *} file: 需要关闭的文件
 * @return        {void}
 **/
void devfs_close(file_t *file)
{
    return;
}
/**
 * @brief        : 跳转到文件指定位置操作
 * @param         {file_t *} file: 需要操作的文件
 * @param         {uint32_t} offset: 偏移量
 * @param         {int} dir:  操作的起始位置
 * @return        {int} : 指定位置的指针
 **/
int devfs_seek(file_t *file, uint32_t offset, int dir)
{
    return 0;
}
/**
 * @brief        : 获取文件状态信息
 * @param         {file_t} *file: 需要获取状态信息的文件对象
 * @param         {stat *} st: 用于存储文件状态信息的结构体指针
 * @return        {int} : 成功返回0，失败返回负数错误码
 **/
int devfs_stat(file_t *file, struct stat *st)
{
    return 0;
}

fs_op_t devfs_op = {
    .mount = devfs_mount,
    .unmount = devfs_unmont,
    .open = devfs_open,
    .read = devfs_read,
    .write = devfs_write,
    .close = devfs_close,
    .seek = devfs_seek,
    .stat = devfs_stat,
};
