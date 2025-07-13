/**
 * @FilePath     : /code/source/kernel/include/fs/fs.h
 * @Description  :  文件系统头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-12 10:06:46
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef FS_H
#define FS_H

#include "comm/types.h"
#include "fs/file.h"
#include "ipc/mutex.h"
#include "tools/list.h"
#include "fs/fatfs/fatfs.h"
#include "applib/lib_syscall.h"
#define FS_TABLE_SIZE 15
#define FS_MOUNT_POINT_SIZE 512 // 挂载点大小
struct stat;

struct _fs_t; // 文件系统前置声明
/**
 * 回调函数表
 */
typedef struct _fs_op_t
{
    /**
     * @brief        : 挂载操作,进行某些初始化
     * @param         {_fs_t} *fs: 指定的文件系统的指针
     * @param         {int} major: 主设备号
     * @param         {int} minor: 次设备号
     * @return        {int} : 成功返回0,失败返回-1
     **/
    int (*mount)(struct _fs_t *fs, int major, int minor);
    /**
     * @brief        : 取消挂载
     * @param         {_fs_t} *fs: 指定的文件系统的指针
     * @return        {void}
     **/
    void (*unmount)(struct _fs_t *fs);
    /**
     * @brief        : 打开某文件系统下的指定路径的文件
     * @param         {_fs_t} *fs: 指定文件系统的指针
     * @param         {char *} path: 文件的路径
     * @param         {file_t *} file: 需要打开的文件
     * @return        {int} : 成功返回0,失败返回-1
     **/
    int (*open)(struct _fs_t *fs, const char *path, file_t *file);
    /**
     * @brief        : 读取某文件
     * @param         {char} *buf: 读入的缓冲区
     * @param         {int} size: 读取的数据量
     * @param         {file_t} *file: 读取的文件
     * @return        {int} : 实际读取的数据量
     **/
    int (*read)(char *buf, int size, file_t *file);
    /**
     * @brief        : 写文件
     * @param         {char} *buf: 写入的数据的指针
     * @param         {int} size: 写入的数据量
     * @param         {file_t *} file: 写入的文件
     * @return        {int} : 实际写入的数据量
     **/
    int (*write)(char *buf, int size, file_t *file);
    /**
     * @brief        : 关闭指定的文件
     * @param         {file_t *} file: 需要关闭的文件
     * @return        {void}
     **/
    void (*close)(file_t *file);
    /**
     * @brief        : 跳转到文件指定位置操作
     * @param         {file_t *} file: 需要操作的文件
     * @param         {uint32_t} offset: 偏移量
     * @param         {int} dir:  操作的起始位置
     * @return        {int} : 指定位置的指针
     **/
    int (*seek)(file_t *file, uint32_t offset, int dir);
    /**
     * @brief        : 获取文件状态信息
     * @param         {file_t} *file: 需要获取状态信息的文件对象
     * @param         {stat *} st: 用于存储文件状态信息的结构体指针
     * @return        {int} : 成功返回0，失败返回负数错误码
     **/
    int (*stat)(file_t *file, struct stat *st);
    /**
     * @brief        : 打开目录
     * @param         {struct _fs_t} *fs: 文件系统对象指针
     * @param         {const char} *name: 要打开的目录名称
     * @param         {DIR} *dir: 用于存储目录信息的DIR结构体指针
     * @return        {int} : 成功返回0，失败返回负数错误码
     **/
    int (*opendir)(struct _fs_t *fs, const char *name, DIR *dir);
    /**
     * @brief        : 读取目录项
     * @param         {struct _fs_t} *fs: 文件系统对象指针
     * @param         {DIR} *dir: 已打开的目录对象指针
     * @param         {struct dirent} *dirent: 用于存储目录项信息的结构体指针
     * @return        {int} : 成功返回0，失败返回负数错误码
     **/
    int (*readdir)(struct _fs_t *fs, DIR *dir, struct dirent *dirent);
    /**
     * @brief        : 关闭目录
     * @param         {struct _fs_t} *fs: 文件系统对象指针
     * @param         {DIR} *dir: 要关闭的目录对象指针
     * @return        {int} : 成功返回0，失败返回负数错误码
     **/
    int (*closedir)(struct _fs_t *fs, DIR *dir);
    /**
     * @brief        : 控制文件输入输出方式
     * @param         {file_t} *file:操作的文件
     * @param         {int} cmd: 控制命令
     * @param         {int} arg0: 参数1
     * @param         {int} arg1: 参数2
     * @return        {int} : 若成功发挥0,失败返回-1
     **/
    int (*ioctl)(file_t *file, int cmd, int arg0, int arg1);
    /**
     * @brief        : 从磁盘中删除文件
     * @param         {_fs_t} *fs: 文件系统描述符的指针
     * @param         {char *} pathname: 路径名称
     * @return        {int} : 成功返回0,失败返回-1
     **/
    int (*unlink)(struct _fs_t *fs, const char *pathname);
} fs_op_t;
typedef enum _fs_type_t
{
    FS_DEVFS, // 设备文件系统
    FS_FAT16, // fat16文件系统
} fs_type_t;
/**
 * 文件系统描述符
 * 如:设备文件系统、磁盘文件系统
 */
typedef struct _fs_t
{
    char mount_point[FS_MOUNT_POINT_SIZE]; // 挂载点
    fs_op_t *op;                           // 指明该文件系统可执行的操作
    void *data;                            // 扩充使用
    int dev_id;                            // 设备id
    list_node_t node;                      // 使用链表链接
    mutex_t *mutex;                        // 互斥锁
    fs_type_t type;                        // 文件类型
    // 临时使用
    union
    {
        fat_t fat_data;
    };
} fs_t;

/**
 * @brief        : 路径转数字
 * @param         {char *} path: 路径
 * @param         {int *} num: 转成数字存储的变量
 * @return        {int} : 成功返回0,失败返回-1
 **/
int path_to_num(const char *path, int *num);
/**
 * @brief        : 获取下一个字路径
 * @param         {char *} path: 传入的路径
 * @return        {const char *} : 下一个路径开始的指针
 **/
const char *path_next_child(const char *path);
/**
 * @brief        : 打开文件
 * @param         {char} *path: 文件路径
 * @param         {int} flags: 标志
 * @return        {int} : 文件id
 **/
int sys_open(const char *path, int flags, ...);
/**
 * @brief        : 读取文件
 * @param         {int} file: 哪一个文件
 * @param         {char} *ptr: 读取到文件的目的地址
 * @param         {int} len: 读取长度
 * @return        {int} 读取成功返回读取长度,失败返回-1
 **/
int sys_read(int file, char *ptr, int len);
/**
 * @brief        : 写文件
 * @param         {int} file: 写入的文件
 * @param         {char} *ptr: 写入的内容的起始地址
 * @param         {int} len: 写入长度
 * @return        {int} : 返回-1
 **/
int sys_write(int file, char *ptr, int len);
/**
 * @brief        : 调整读写指针
 * @param         {int} file: 操作文件
 * @param         {int} ptr: 要调整的指针
 * @param         {int} dir: 文件所在目录
 * @return        {int} 成功返回 1, 失败返回 -1
 **/
int sys_lseek(int file, int ptr, int dir);
/**
 * @brief        : 关闭某个打开的文件
 * @param         {int} file: 操作的文件
 * @return        {int} : 返回0
 **/
int sys_close(int file);
/**
 * @brief        : 判断文件是否为tty设备
 * @param         {int} file: 操作的文件
 * @return        {int} : 返回0
 **/
int sys_isatty(int file);
/**
 * @brief        : 获取文件状态信息
 * @param         {int} file: 文件描述符
 * @param         {struct stat} *st: 存储文件信息的结构体指针
 * @return        {int} 成功返回0，失败返回-1
 **/
int sys_fstat(int file, struct stat *st);
/**
 * @brief        : 复制文件描述符
 * @param         {int} file: 要复制的文件描述符
 * @return        {int} 新的文件描述符，失败返回-1
 **/
int sys_dup(int file);
/**
 * @brief        : 文件系统初始化
 * @return        {void}
 **/
void fs_init(void);

int sys_opendir(const char *path, DIR *dir);

int sys_readdir(DIR *dir, struct dirent *dirent);

int sys_closedir(DIR *dir);

void sys_ioctl(int file, int cmd, int arg0, int arg1); // 输入输出控制
int sys_unlink(const char *pathname);

#endif