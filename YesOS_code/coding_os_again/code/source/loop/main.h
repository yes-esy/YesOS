/**
 * @FilePath     : /code/source/shell/main.h
 * @Description  :  shell头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-09 10:53:42
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef MAIN_H
#define MAIN_H

// command line interface
#define CLI_INPUT_SIZE 1024  // 命令行输入缓冲区
#define CLI_MAX_ARG_COUNT 10 // 最大参数个数、

#define ESC_CMD2(Pn, cmd) "\x1b[" #Pn #cmd
#define ESC_COLOR_ERROR ESC_CMD2(31, m)   // 红色错误
#define ESC_COLOR_SUCCESS ESC_CMD2(32, m) // 绿色成功
#define ESC_COLOR_WARNING ESC_CMD2(33, m) // 黄色警告
#define ESC_COLOR_INFO ESC_CMD2(34, m)    // 蓝色信息
#define ESC_COLOR_PROMPT ESC_CMD2(36, m)  // 青色提示符
#define ESC_COLOR_HELP ESC_CMD2(35, m)    // 紫色帮助
#define ESC_COLOR_DEFAULT ESC_CMD2(39, m) // 默认颜色
#define ESC_COLOR_BOLD ESC_CMD2(1, m)     // 粗体
#define ESC_COLOR_RESET "\x1b[0m"         // 重置所有格式
#define ESC_CLEAR_SCREEN ESC_CMD2(2, J)   // 擦除整屏幕
#define ESC_MOVE_CURSOR(row, col) "\x1b[" #row ";" #col "H"


#endif