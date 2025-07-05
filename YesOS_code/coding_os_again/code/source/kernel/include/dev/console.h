/**
 * @FilePath     : /code/source/kernel/include/dev/console.h
 * @Description  :  控制台头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-05 15:08:40
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef CONSOLE_H
#define CONSOLE_H

#include "comm/types.h"
#define CONSOLE_DISP_ADDR 0xb8000              // 显存起始地址
#define CONSOLE_DISP_END (0xb8000 + 32 * 1024) // 显存结束地址
#define CONSOLE_ROW_MAX 25                     // 最大行数
#define CONSOLE_COL_MAX 80                     // 最大列数
#define CONSOLE_NR 1                           // 控制台的数量

// 各种颜色
typedef enum _cclor_t
{
    COLOR_Black = 0,
    COLOR_Blue = 1,
    COLOR_Green = 2,
    COLOR_Cyan = 3,
    COLOR_Red = 4,
    COLOR_Magenta = 5,
    COLOR_Brown = 6,
    COLOR_Gray = 7,
    COLOR_Dark_Gray = 8,
    COLOR_Light_Blue = 9,
    COLOR_Light_Green = 10,
    COLOR_Light_Cyan = 11,
    COLOR_Light_Red = 12,
    COLOR_Light_Magenta = 13,
    COLOR_Yellow = 14,
    COLOR_White = 15
} cclor_t;
/**
 * 描述显示字符结构
 */
typedef union _disp_char_t
{
    struct
    {
        char c;
        char foreground : 4;
        char background : 3;
    };
    uint16_t v; // 一个字符
} disp_char_t;

/**
 * 控制台
 */
typedef struct _console_t
{
    disp_char_t *disp_base; // 起始地址
    int display_rows;       // 行数
    int display_cols;       // 列数
    int cursor_row;         // 光标所在行
    int cursor_col;         // 关标所在列
    cclor_t foreground;     // 前景色
    cclor_t background;     // 后景色
} console_t;

/**
 * @brief        : 初始化控制台
 * @return        {int} :返回0
 **/
int console_init(void);

/**
 * @brief        : 往控制台写数据
 * @param         {int} console: 哪一个控制台
 * @param         {char *} data: 要写的数据
 * @param         {int} size: 大小
 * @return        {int} 返回0
 **/
int console_write(int console_id, char *data, int size);
/**
 * @brief        : 关闭控制台
 * @param         {int} console_id: 控制台id
 * @return        {*}
 **/
void console_close(int console_id);
#endif