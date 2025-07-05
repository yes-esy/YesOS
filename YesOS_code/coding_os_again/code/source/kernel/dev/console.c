/**
 * @FilePath     : /code/source/kernel/dev/console.c
 * @Description  :  控制台实现函数
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-05 16:11:08
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "dev/console.h"
#include "tools/klib.h"
#include "comm/cpu_instr.h"
/**
 * @brief        : 读取光标的位置
 * @return        {int} : 光标的索引
 **/
static int read_cursor_pos(void)
{
    int pos;
    outb(0x3D4, 0xF);
    pos = inb(0x3D5);
    outb(0x3D4, 0xE);
    pos |= inb(0x3D5) << 8;
    return pos;
}

/**
 * @brief        : 写入光标的位置
 * @return        {int} : 光标的索引
 **/
static int update_cursor_pos(console_t *console)
{
    uint16_t pos = console->cursor_row * console->display_cols + console->cursor_col; // 获取光标位置

    outb(0x3D4, 0xF);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0xE);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    return pos;
}

static console_t console_buff[CONSOLE_NR]; // 控制台
/**
 * @brief        : 清空指定起始至结束行
 * @param         {console_t} *console: 操作的控制台
 * @param         {int} start: 清空的起始行
 * @param         {int} end: 清空的结束行
 * @return        {void}
 **/
static void erase_rows(console_t *console, int start, int end)
{
    disp_char_t *disp_start = console->disp_base + console->display_cols * start;   // 擦除起始地址
    disp_char_t *disp_end = console->disp_base + console->display_cols * (end + 1); // 擦除的结束地址

    while (disp_start < disp_end)
    {
        disp_start->c = ' '; // 置为空
        disp_start->background = console->background;
        disp_start->foreground = console->foreground;

        disp_start++;
    }
}
/**
 * @brief        : 往上滚动,往前拷贝
 * @param         {console_t *} console: 要操作的控制台
 * @param         {int} lines: 上滚的行数
 * @return        {void}
 **/
static void scroll_up(console_t *console, int lines)
{
    disp_char_t *dest = console->disp_base;                                                        // 目标位置，从屏幕第一行开始
    disp_char_t *src = console->disp_base + console->display_cols * lines;                         // 源位置，从要保留的第一行开始（跳过要删除的行数）
    uint32_t size = (console->display_rows - lines) * console->display_cols * sizeof(disp_char_t); // 拷贝大小
    kernel_memcpy((void *)dest, (void *)src, size);
    erase_rows(console, console->display_rows - lines, console->display_rows - 1); // 擦除已经拷贝的行
    console->cursor_row -= lines;
}
/**
 * @brief        : 将当前光标移动到第一列
 * @param         {console_t} *console: 需要操作的控制台
 * @return        {void}
 **/
static void move_to_col0(console_t *console)
{
    console->cursor_col = 0;
}
/**
 * @brief        : 将光标移动到下一行
 * @param         {console_t*} console: 要操作的控制台
 * @return        {void}
 **/
static void move_next_line(console_t *console)
{
    console->cursor_row++;
    if (console->cursor_row >= console->display_rows) // 超过最大行数
    {
        scroll_up(console, 1); // 上滚一行
    }
}
/**
 * @brief        : 往前移动关标
 * @param         {console_t} *console: 操作哪一个控制台
 * @param         {int} n: 移动的位置
 * @return        {int}
 **/
void move_forward(console_t *console, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (++console->cursor_col >= console->display_cols) // 超过最大列数,移动到下一行起始位置
        {
            console->cursor_row++;                            // 下一行
            console->cursor_col = 0;                          // 第0列
            if (console->cursor_row >= console->display_rows) // 超过最大行数,上滚一行
            {
                scroll_up(console, 1);
            }
        }
    }
}
/**
 * @brief        : 将字符输出到控制台
 * @param         {console_t *} console: 目的控制台指针
 * @param         {char} c: 字符
 * @return        {void}
 **/
static void show_char(console_t *console, char c)
{
    int offset = console->cursor_col + console->cursor_row * console->display_cols; // 偏移量
    disp_char_t *disp_start = console->disp_base + offset;                          // 起始地址
    disp_start->c = c;
    // 设置颜色
    disp_start->foreground = console->foreground;
    disp_start->background = console->background;
    move_forward(console, 1); // 前移
}
/**
 * @brief        : 清空控制台
 * @param         {console_t} *console: 需要清空的控制台指针
 * @return        {void}
 **/
static void clear_display(console_t *console)
{
    int size = console->display_cols * console->display_rows; // 清空的大小
    disp_char_t *disp_start = console->disp_base;             // 起始地址
    for (int i = 0; i < size; i++, disp_start++)
    {
        disp_start->c = ' ';
        disp_start->background = console->background;
        disp_start->foreground = console->foreground;
    }
}
/**
 * @brief        : 初始化控制台
 * @return        {int} :返回0
 **/
int console_init(void)
{
    for (int i = 0; i < CONSOLE_NR; i++)
    {
        console_t *console = console_buff + i;   // 第几个控制台
        console->display_cols = CONSOLE_COL_MAX; // 行数
        console->display_rows = CONSOLE_ROW_MAX; // 列数
        console->foreground = COLOR_White;
        console->background = COLOR_Black;
        int cursor_pos = read_cursor_pos();                       // 读取光标位置
        console->cursor_row = cursor_pos / console->display_cols; // 当前关标行号
        console->cursor_col = cursor_pos % console->display_cols; // 当前光标列号
        console->disp_base = (disp_char_t *)CONSOLE_DISP_ADDR + i * (CONSOLE_COL_MAX * CONSOLE_ROW_MAX);
        // clear_display(console);
    }
    return 0;
}
/**
 * @brief        : 往控制台写数据
 * @param         {int} console: 哪一个控制台
 * @param         {char *} data: 要写的数据
 * @param         {int} size: 大小
 * @return        {int} 返回写入的长度
 **/
int console_write(int console_id, char *data, int size)
{
    console_t *console = console_buff + console_id; // 获取当前控制台;

    int len;
    for (len = 0; len < size; len++)
    {
        char ch = *data++;
        switch (ch)
        {
        case '\n':
            move_to_col0(console);
            move_next_line(console);
            break;
        default:
            show_char(console, ch);
            break;
        }
        // @todo
    }
    update_cursor_pos(console); // 更新光标位置
    return len;
}
/**
 * @brief        : 关闭控制台
 * @param         {int} console_id: 控制台id
 * @return        {*}
 **/
void console_console(int console_id)
{
}