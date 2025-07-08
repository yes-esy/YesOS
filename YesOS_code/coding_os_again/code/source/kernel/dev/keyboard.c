/**
 * @FilePath     : /code/source/kernel/dev/keyboard.c
 * @Description  :  键盘相关实现c文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-07 10:37:19
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "dev/keyboard.h"
#include "cpu/irq.h"
#include "comm/cpu_instr.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "dev/tty.h"
/**
 * 键盘映射表，分3类
 * normal是没有shift键按下，或者没有numlock按下时默认的键值
 * func是按下shift或者numlock按下时的键值
 * esc是以esc开头的的键值
 */
static kbd_state_t kbd_state;
static const key_map_t map_table[] =
    {
        [0x2] = {'1', '!'},
        [0x3] = {'2', '@'},
        [0x4] = {'3', '#'},
        [0x5] = {'4', '$'},
        [0x6] = {'5', '%'},
        [0x7] = {'6', '^'},
        [0x08] = {'7', '&'},
        [0x09] = {'8', '*'},
        [0x0A] = {'9', '('},
        [0x0B] = {'0', ')'},
        [0x0C] = {'-', '_'},
        [0x0D] = {'=', '+'},
        [0x0E] = {0x7F, 0x7F},
        [0x0F] = {'\t', '\t'},
        [0x10] = {'q', 'Q'},
        [0x11] = {'w', 'W'},
        [0x12] = {'e', 'E'},
        [0x13] = {'r', 'R'},
        [0x14] = {'t', 'T'},
        [0x15] = {'y', 'Y'},
        [0x16] = {'u', 'U'},
        [0x17] = {'i', 'I'},
        [0x18] = {'o', 'O'},
        [0x19] = {'p', 'P'},
        [0x1A] = {'[', '{'},
        [0x1B] = {']', '}'},
        [0x1C] = {'\n', '\n'},
        [0x1E] = {'a', 'A'},
        [0x1F] = {'s', 'B'},
        [0x20] = {'d', 'D'},
        [0x21] = {'f', 'F'},
        [0x22] = {'g', 'G'},
        [0x23] = {'h', 'H'},
        [0x24] = {'j', 'J'},
        [0x25] = {'k', 'K'},
        [0x26] = {'l', 'L'},
        [0x27] = {';', ':'},
        [0x28] = {'\'', '"'},
        [0x29] = {'`', '~'},
        [0x2B] = {'\\', '|'},
        [0x2C] = {'z', 'Z'},
        [0x2D] = {'x', 'X'},
        [0x2E] = {'c', 'C'},
        [0x2F] = {'v', 'V'},
        [0x30] = {'b', 'B'},
        [0x31] = {'n', 'N'},
        [0x32] = {'m', 'M'},
        [0x33] = {',', '<'},
        [0x34] = {'.', '>'},
        [0x35] = {'/', '?'},
        [0x39] = {' ', ' '},
}; // 键盘映射表
/**
 * @brief        : 判断按键是否为按下
 * @param         {uint8_t} key_code: 键盘的原始值
 * @return        {int} : 1是,0否
 **/
static inline int is_make_code(uint8_t key_code)
{
    return !(key_code & 0x80);
}
/**
 * @brief        : 去掉键盘对应的值的最高位
 * @param         {uint8_t} key_code: 键盘的原始值
 * @return        {char} 键盘映射后的值
 **/
static inline char get_key(uint8_t key_code)
{
    return key_code & 0x7f;
}
/**
 * @brief        : 等待可写数据
 * @return        {*}
 **/
void kbd_wait_send_ready()
{
    uint32_t time_out = 10000;
    while (time_out--)
    {
        if ((inb(KBD_PORT_STAT) & KBD_STAT_SEND_FULL) == 0)
        {
            return;
        }
    }
}
/**
 * @brief        : 等待可用的键盘数据
 * @return        {void}
 **/
void kbd_wait_recv_ready(void)
{
    uint32_t time_out = 100000;
    while (time_out--)
    {
        if (inb(KBD_PORT_STAT) & KBD_STAT_RECV_READY)
        {
            return;
        }
    }
}
/**
 * @brief        : 向键盘指定端口写数据
 * @param         {uint8_t} port: 端口号
 * @param         {uint8_t} data: 数据
 * @return        {void}
 **/
void kbd_write(uint8_t port, uint8_t data)
{
    kbd_wait_send_ready(); // 等待就绪信号
    outb(port, data);
}
/**
 * @brief        : 读取键盘数据
 * @return        {uint8_t}: 读取到的键盘的数据
 **/
uint8_t kbd_read(void)
{
    kbd_wait_recv_ready();
    return inb(KBD_PORT_DATA);
}
/**
 * @brief        : 更新键盘指示灯
 * @return        {void}
 **/
static void update_led_status(void)
{
    int data = 0;

    data = (kbd_state.caps_lock ? 1 : 0) << 0;
    kbd_write(KBD_PORT_DATA, KBD_CMD_RW_LED);
    kbd_write(KBD_PORT_DATA, data);
    kbd_read();
}
static void do_e0_key(uint8_t raw_code)
{
    char key = get_key(raw_code); // 去掉最高位
    int is_make = is_make_code(raw_code);
    switch (key)
    {
    case KEY_CTRL:
        kbd_state.rctrl_press = is_make;
        break;
    case KEY_ALT:
        kbd_state.ralt_press = is_make;
        break;
    default:
        break;
    }
}
/**
 * @brief        : 处理ctrl+f功能键
 * @param         {int} key: f功能键键值f1-f8
 * @return        {void}
 **/
void do_fx_key(int key)
{
    int index = key - KEY_F1;                           // 转换成tty设备(屏幕)索引
    if (kbd_state.lctrl_press || kbd_state.rctrl_press) // ctrl键按下
    {
        tty_select(index);
    }
}
/**
 * @brief        : 处理单字符的标准键
 * @param         {uint8_t} raw_code: 键盘原始值
 * @return        {void}
 **/
static void
do_normal_key(uint8_t raw_code)
{
    char key = get_key(raw_code); // 去掉最高位
    int is_make = is_make_code(raw_code);
    switch (key)
    {
    case KEY_RSHIFT: // 右shift键按下
        kbd_state.rshift_press = is_make ? 1 : 0;
        break;
    case KEY_LSHIFT: // 左shift键按下
        kbd_state.lshift_press = is_make ? 1 : 0;
        break;
    case KEY_CAPSLOCK: // capslk键按下
        if (is_make)
        {
            kbd_state.caps_lock = ~kbd_state.caps_lock; // 取反
            update_led_status();                        // 更新灯
        }
        break;
    case KEY_ALT:
        kbd_state.lalt_press = is_make;
        break;
    case KEY_CTRL:
        kbd_state.lctrl_press = is_make;
        break;
        //@todo 功能键：写入键盘缓冲区，后续完善
    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
    case KEY_F5:
    case KEY_F6:
    case KEY_F7:
    case KEY_F8:
        do_fx_key(key); // 特殊功能键
        break;
    case KEY_F9:
    case KEY_F10:
    case KEY_F11:
    case KEY_F12:
    case KEY_SCROLL_LOCK:
    default:
        if (is_make)
        {
            if (kbd_state.rshift_press || kbd_state.lshift_press)
            {
                key = map_table[key].func;
            }
            else
            {
                key = map_table[key].normal;
            }

            if (kbd_state.caps_lock) // capslk键按下
            {
                if ((key >= 'A') && (key <= 'Z')) // 原来为大写需转为小写
                {
                    key = key - 'A' + 'a'; // 转为小写
                }
                else if ((key >= 'a') && (key <= 'z')) // 原来为小写需转为大写
                {
                    key = key - 'a' + 'A';
                }
            }
            // log_printf("press key is %c\n", key);
            tty_in( key); // 传递给tty设备
        }
        break;
    }
}
/**
 * @brief        : 初始化
 * @return        {*}
 **/
void kbd_init(void)
{
    static int inited = 0; // 是否已经被初始化
    if (!inited)           // 已经初始化
    {
        kernel_memset((void *)&kbd_state, 0, sizeof(kbd_state_t));
        irq_install(IRQ1_KEYBOARD, (irq_handler_t)exception_handler_kbd); // 安装键盘处理程序
        irq_enable(IRQ1_KEYBOARD);                                        // 打开键盘中断
        inited = 1;                                                       // 初始化次数为1
    }
}
/**
 * @brief        : 中断服务子程序
 * @param         {exception_frame_t *} frame: 函数栈帧
 * @return        {void}
 **/
void do_handler_kbd(exception_frame_t *frame)
{
    static enum {
        NORMAL,
        BEGIN_E0,
        BEGIN_E1,
    } recv_state = NORMAL;                // 键盘初始状态
    uint32_t status = inb(KBD_PORT_STAT); // 读取状态,检测是否有程序
    if (!(status & KBD_STAT_RECV_READY))  // 非键盘中断
    {
        pic_send_eoi(IRQ1_KEYBOARD);
        return;
    }
    uint8_t raw_code = inb(KBD_PORT_DATA); // 读取键盘的值
    // do_normal_key(raw_code);
    // log_printf("key %d\n", raw_code);
    pic_send_eoi(IRQ1_KEYBOARD); // 读取完成之后，就可以发EOI，方便后续继续响应键盘中断

    if (raw_code == KEY_E0) // E0字符
    {
        recv_state = BEGIN_E0;
    }
    else if (raw_code == KEY_E1) // E1字符
    {
        recv_state = BEGIN_E1;
    }
    else
    {
        switch (recv_state)
        {
        case NORMAL:
            do_normal_key(raw_code);
            break;
        case BEGIN_E0: // 不处理print scr
            do_e0_key(raw_code);
            recv_state = NORMAL;
            break;
        case BEGIN_E1: // 不处理pause
            recv_state = NORMAL;
            break;
        }
    }
}