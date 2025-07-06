/**
 * @FilePath     : /code/source/kernel/include/fs/fs.h
 * @Description  :  文件系统头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-06 15:26:08
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
**/
#ifndef FS_H
#define FS_H

#include "comm/types.h"
struct stat;
/**
 * @brief        : 打开文件
 * @param         {char} *path: 文件路径
 * @param         {int} flags: 标志
 * @return        {int} : 文件id
 **/
int sys_open(const char * path, int flags,...);
/**
 * @brief        : 读取文件
 * @param         {int} file: 哪一个文件
 * @param         {char} *ptr: 读取到文件的目的地址
 * @param         {int} len: 读取长度
 * @return        {int} 读取成功返回读取长度,失败返回-1
 **/
int sys_read(int file,char *ptr , int len);
/**
 * @brief        : 写文件
 * @param         {int} file: 写入的文件
 * @param         {char} *ptr: 写入的内容的起始地址
 * @param         {int} len: 写入长度
 * @return        {int} : 返回-1
 **/
int sys_write(int file,char *ptr , int len);
/**
 * @brief        : 调整读写指针
 * @param         {int} file: 操作文件
 * @param         {int} ptr: 要调整的指针
 * @param         {int} dir: 文件所在目录
 * @return        {int} 成功返回 1, 失败返回 -1
 **/
int sys_lseek(int file,int ptr , int dir);
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
 * @brief        :
 * @param         {stat} *st:
 * @return        {*}
 **/
int sys_fstat(struct stat * st);
#endif