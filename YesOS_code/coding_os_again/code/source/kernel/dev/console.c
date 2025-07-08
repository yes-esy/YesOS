/**
 * @FilePath     : /code/source/kernel/dev/console.c
 * @Description  :  控制台实现函数
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-08 15:44:34
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "dev/console.h"
#include "tools/klib.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
static int curr_console_index;             // 当前控制台索引
static console_t console_buff[CONSOLE_NR]; // 控制台
/**
 * @brief        : 读取光标的位置
 * @return        {int} : 光标的索引
 **/
static int read_cursor_pos(void)
{
    int pos;                                    // 记录关标位置
    irq_state_t state = irq_enter_protection(); // 开中断保护
    outb(0x3D4, 0xF);
    pos = inb(0x3D5);
    outb(0x3D4, 0xE);
    pos |= inb(0x3D5) << 8;
    irq_leave_protection(state); // 关中断返回
    return pos;
}

/**
 * @brief        : 写入光标的位置
 * @return        {int} : 光标的索引
 **/
static int update_cursor_pos(console_t *console)
{
    uint16_t pos = (console - console_buff) * console->display_cols * console->display_rows; // 用于记录关标位置
    pos += console->cursor_row * console->display_cols + console->cursor_col;                // 获取光标位置
    irq_state_t state = irq_enter_protection();                                              // 开中断保护
    outb(0x3D4, 0xF);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0xE);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    irq_leave_protection(state); // 关中断返回
    return pos;
}
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
 * @brief        : 光标左移
 * @param         {console_t} console: 操作的控制台
 * @param         {int} n: 左移的单位
 * @return        {int} : 状态码-1失败1成功
 **/
static int move_backword(console_t *console, int n)
{
    int status = -1;
    for (int i = 0; i < n; i++)
    {
        if (console->cursor_col > 0)
        {
            console->cursor_col--; // 光标左移
            status = 0;
        }
        else if (console->cursor_row > 0)
        {
            console->cursor_row--;
            console->cursor_col = console->display_cols - 1;
            status = 0;
        }
    }
    return status;
}
/**
 * @brief        : 光标往左擦除一个单位
 * @param         {console_t *} console: 操作的控制台
 * @return        {void}
 **/
static void erase_backword(console_t *console)
{
    if (move_backword(console, 1) == 0)
    {
        show_char(console, ' ');
        move_backword(console, 1);
    }
}
/**
 * @brief        : 向控制台输出正常字符
 * @param         {console_t *} console: 操作的控制台
 * @param         {char} ch: 输出的字符
 * @return        {void}
 **/
static void write_normal(console_t *console, char ch)
{
    switch (ch)
    {
    case ASCII_ESC:
        console->write_state = CONSOLE_WRITE_ESC; // 状态调整为写esc字符
        break;
    case 0x7F:
        erase_backword(console); // 往前删除一个字符
        break;
    case '\b':
        move_backword(console, 1); // 往左移动一个光标
        break;
    case '\r':
        move_to_col0(console); // 回车
        break;
    case '\n': // 换行
        // move_to_col0(console);
        move_next_line(console);
        break;
    default:
        if ((ch >= ' ') && (ch <= '~')) // 可打印
        {
            show_char(console, ch);
        }
        break;
    }
}
/**
 * @brief        : 保存光标
 * @param         {console_t} *console: 需要操作的控制台
 * @return        {void}
 **/
static void save_cursor(console_t *console)
{
    console->old_cursor_col = console->cursor_col; // 保存光标行
    console->old_cursor_row = console->cursor_row; // 保存光标列
}
/**
 * @brief        : 恢复光标
 * @param         {console_t} *console: 需要操作的控制台
 * @return        {void}
 **/
static void restore_cusor(console_t *console)
{
    console->cursor_col = console->old_cursor_col; // 恢复光标行
    console->cursor_row = console->old_cursor_row; // 恢复光标列
}
static void clear_esc_param(console_t *console)
{
    kernel_memset((void *)console->esc_param, 0, sizeof(console->esc_param)); // 清空参数列表
    console->curr_param_index = 0;
}
/**
 * @brief        : 向控制台输出esc字符
 * @param         {console_t} *console: 需要输出的控制台
 * @param         {char} ch: 需要输出的字符
 * @return        {void}
 **/
static void write_esc(console_t *console, char ch)
{
    switch (ch)
    {
    case '7':
        save_cursor(console);                        // 保存光标
        console->write_state = CONSOLE_WRITE_NORMAL; // 恢复为正常状态
        break;
    case '8':
        restore_cusor(console);                      // 恢复光标
        console->write_state = CONSOLE_WRITE_NORMAL; // 调整状态
        break;
    case '[':                                        // 遇到左括号
        clear_esc_param(console);                    // 清空参数列表
        console->write_state = CONSOLE_WRITE_SQUARE; // 调整状态
        break;
    default:
        console->write_state = CONSOLE_WRITE_NORMAL;
        break;
    }
}
/**
 * @brief        : 设置输出字体样式
 * @param         {console_t} *console: 要输出的控制台
 * @return        {void}
 **/
static void set_font_style(console_t *console)
{
    static const cclor_t color_table[] = {
        COLOR_Black, COLOR_Red, COLOR_Green, COLOR_Yellow,  // 0-3
        COLOR_Blue, COLOR_Magenta, COLOR_Cyan, COLOR_White, // 4-7
    };

    for (int i = 0; i < console->curr_param_index; i++)
    {
        int param = console->esc_param[i];
        if ((param >= 30) && (param <= 37))
        { // 前景色：30-37
            console->foreground = color_table[param - 30];
        }
        else if ((param >= 40) && (param <= 47))
        {
            console->background = color_table[param - 40];
        }
        else if (param == 39)
        { // 39=默认前景色
            console->foreground = COLOR_White;
        }
        else if (param == 49)
        { // 49=默认背景色
            console->background = COLOR_Black;
        }
    }
}
/**
 * @brief        : 擦除控制台部分内容
 * @param         {console_t} *console: 要操作的控制台
 * @return        {void}
 **/
static void erase_in_display(console_t *console)
{
    if (console->curr_param_index < 0) // 索引错误
    {
        return;
    }
    int param = console->esc_param[0]; // 取第一个参数
    if (param == 2)
    {
        erase_rows(console, 0, console->display_rows - 1);
        console->cursor_col = console->cursor_row = 0;
    }
}
/**
 * @brief        : 关标右移
 * @param         {console_t} *console: 操作的控制台
 * @param         {int} n: 移动的单位数
 * @return        {void}
 **/
static void move_left(console_t *console, int n)
{
    if (n == 0)
    {
        n = 1;
    }

    int col = console->cursor_col - n; // 光标移动后的列数
    console->cursor_col = (col >= 0) ? col : 0;
}
/**
 * @brief        : 光标左移
 * @param         {console_t} *console: 要操作的控制台
 * @param         {int} n: 移动的位置
 * @return        {void}
 **/
static void move_right(console_t *console, int n)
{
    if (n == 0)
    {
        n = 1;
    }

    int col = console->cursor_col + n; // 光标移动后的列数
    if (col >= console->display_cols)
    {
        console->cursor_col = console->display_cols - 1; // 移动到最后一列
    }
    else
    {
        console->cursor_col = col;
    }
}
/**
 * @brief        : 移动关标
 * @param         {console_t} *console: 操作的控制台
 * @return        {void}
 **/
static void move_cursor(console_t *console)
{
    console->cursor_row = console->esc_param[0]; // 光标行指定位置
    console->cursor_col = console->esc_param[1]; // 关标列指定位置
}
/**
 * @brief        : 向控制台输出遇到左括号的时的特殊字符
 * @param         {console_t} *console: 操作的控制台
 * @param         {char} c: 输出的字符
 * @return        {void}
 **/
static void write_esc_square(console_t *console, char c)
{
    if ((c >= '0') && (c <= '9')) // 参数形式正确
    {
        int *param = &console->esc_param[console->curr_param_index]; // 当前参数指针
        *param = *param * 10 + c - '0';                              // 转换为实际的数字
    }
    else if ((c == ';') && (console->curr_param_index < ESC_PARAM_MAX)) // 遇到；号
    {
        console->curr_param_index++;
    }
    else
    {
        // 结束上一字符的处理
        console->curr_param_index++;
        switch (c)
        {
        case 'm':
            set_font_style(console);
            break;
        case 'D':
            move_left(console, console->esc_param[0]);
            break;
        case 'C':
            move_right(console, console->esc_param[0]);
            break;
        case 'H':
        case 'f':
            move_cursor(console);
            break;
        case 'J':
            erase_in_display(console);
        default:
            break;
        }
        console->write_state = CONSOLE_WRITE_NORMAL;
    }
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
 * @param         {int} index : 初始化控制台的索引
 * @return        {int} :返回0
 **/
int console_init(int index)
{

    console_t *console = console_buff + index; // 第几个控制台
    console->display_cols = CONSOLE_COL_MAX;   // 行数
    console->display_rows = CONSOLE_ROW_MAX;   // 列数
    console->foreground = COLOR_White;
    console->background = COLOR_Black;
    console->disp_base = (disp_char_t *)CONSOLE_DISP_ADDR + index * (CONSOLE_COL_MAX * CONSOLE_ROW_MAX);
    if (index == 0) // 第0块屏幕
    {
        int cursor_pos = read_cursor_pos();                       // 读取光标位置
        console->cursor_row = cursor_pos / console->display_cols; // 当前关标行号
        console->cursor_col = cursor_pos % console->display_cols; // 当前光标列号
    }
    else
    {
        // 从头开始
        console->cursor_row = 0;
        console->cursor_col = 0;
        clear_display(console); // 清空屏幕
        // update_cursor_pos(console); // 更新光标位置
    }
    console->old_cursor_col = console->cursor_col;
    console->old_cursor_row = console->cursor_row; // 保存关标的位置
    console->write_state = CONSOLE_WRITE_NORMAL;   // 初始状态写普通字符
    // clear_display(console);
    return 0;
}
/**
 * @brief        : 往控制台写数据
 * @param         {tty_t *} tty: tty设备的指针
 * @return        {int} 返回写入的长度
 **/
int console_write(tty_t *tty)
{
    console_t *console = console_buff + tty->console_index; // 获取当前控制台;

    int len = 0; // 记录写出数据的长度
    do
    {
        char ch;                                   // 记录读取数据
        int err = tty_fifo_get(&tty->O_fifo, &ch); // 从输出缓冲队列读取数据
        if (err < 0)                               // 读取失败
        {
            break;
        }
        sem_signal(&tty->o_sem); //  取完了,发信号量
        switch (console->write_state)
        {
        case CONSOLE_WRITE_NORMAL: // 写普通字符
            write_normal(console, ch);
            break;
        case CONSOLE_WRITE_ESC: // 写特殊字符
            write_esc(console, ch);
            break;
        case CONSOLE_WRITE_SQUARE: // 写esc参数列表
            write_esc_square(console, ch);
        default:
            break;
        }
        len++;
        // @todo
    } while (1);
    if (tty->console_index == curr_console_index)
    {
        update_cursor_pos(console); // 更新光标位置
    }
    return len;
}
/**
 * @brief        : 切换特定显示数据
 * @param         {int} tty_index: 指定tty设备索引
 * @return        {void}
 **/
void console_select(int tty_index)
{
    console_t *console = console_buff + tty_index; // 获取当前的tty显示设备
    if (console->disp_base == 0)                   // 当前设备还未打开
    {
        console_init(tty_index); // 打开并初始化当前tty设备
    }
    uint16_t pos = tty_index * console->display_rows * console->display_cols; // 当前控制台显示的地址
    irq_state_t state = irq_enter_protection();                               // 开启中断保护
    outb(0x3D4, 0xC);                                                         // 往0x3D4写入数据
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));                                // 往0x3D5先写入高8位
    outb(0x3D4, 0xD);                                                         // 往0x3D4写入数据
    outb(0x3D5, (uint8_t)(pos & 0xFF));                                       // 往0x3D5写入低8位
    irq_leave_protection(state);
    curr_console_index = tty_index;
    update_cursor_pos(console); // 更新光标到当前屏幕
}
/**
 * @brief        : 关闭控制台
 * @param         {int} console_id: 控制台id
 * @return        {*}
 **/
void console_close(int console_id)
{
}