/**
 * @FilePath     : /code/source/kernel/include/tools/log.h
 * @Description  :  日志输出相关的头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-17 16:09:23
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
**/
#ifndef LOG_H
#define LOG_H

void log_init(void); // 日志输出初始化函数
void log_printf(const char * fmt,...);  //日志输出接口

#endif
