/**
 * @FilePath     : /code/source/kernel/include/fs/file.h
 * @Description  :  文件相关头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-09 17:11:43
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef FILE_H
#define FILE_H
#include "comm/types.h"

#define FILE_NAME_SIZE 64    // 文件大小
#define FILE_TABLE_SIZE 2048 // 文件表大小
/**
 * 文件类型
 */
typedef enum _file_type_t
{
    FILE_UNKNOWN = 0,
    FILE_TTY,
} file_type_t;
struct _fs_t; // 文件系统前置声明
/**
 * 文件描述结构
 */
typedef struct _file_t
{
    char file_name[FILE_NAME_SIZE]; // 文件名
    file_type_t type;               // 文件类型
    uint32_t size;                  // 文件大小
    int ref;                        // 打开次数
    int dev_id;                     // 所属设备id
    int pos;                        // 读取到的位置
    int mode;                       // 读写模式
    struct _fs_t *fs;               // 文件系统
} file_t;

/**
 * @brief        : 对文件表初始化
 * @return        {void}
 **/
void file_table_init(void);
/**
 * @brief        : 从文件表中分配一个空闲表项
 * @return        {file_t*} : 分配的表项的指针
 **/
file_t *file_alloc(void);
/**
 * @brief        : 从文件表释放文件
 * @param         {file_t *} file: 要释放的文件
 * @return        {void}
 **/
void file_free(file_t *file);
/**
 * @brief        : 增加文件的使用次数
 * @param         {file_t *} file: 需要增加的文件
 * @return        {void}
 **/
void file_incr_ref(file_t *file);
#endif
