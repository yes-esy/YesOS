/**
 * @FilePath     : /code/source/kernel/include/dev/keyboard.h
 * @Description  :  键盘头相关文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-06 14:36:25
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#ifndef KBD_H
#define KBD_H
#include "cpu/irq.h"
#define KBD_PORT_DATA 0x60 // 数据相关端口
#define KBD_PORT_STAT 0x64 // 读状态相关端口
#define KBD_PORT_CMD 0x64  // 写

#define KBD_STAT_RECV_READY (1 << 0) // 是否为键盘中断
#define KBD_STAT_SEND_FULL (1 << 1)  // 键盘发送完毕

#define KEY_RSHIFT 0x36     // 右shift键
#define KEY_LSHIFT 0x2A     // 左shift键
#define KEY_CAPSLOCK 0x3A   // CapsLock键
#define KBD_CMD_RW_LED 0xED // 键盘指示灯

#define KEY_E0 0xE0 // E0开头的键的值
#define KEY_E1 0xE1 // E1开头的键的值

#define KEY_CTRL 0x1D   // CTRL键
#define KEY_ALT 0x38    // ALT键
#define KEY_FUNC 0x8000 //
#define KEY_F1 (0x3B)   // F1键
#define KEY_F2 (0x3C)   // F2键
#define KEY_F3 (0x3D)   // F3键
#define KEY_F4 (0x3E)   // F4键
#define KEY_F5 (0x3F)   // F5键
#define KEY_F6 (0x40)   // F6键
#define KEY_F7 (0x41)   // F7键
#define KEY_F8 (0x42)   // F8键
#define KEY_F9 (0x43)   // F9键
#define KEY_F10 (0x44)  // F10键
#define KEY_F11 (0x57)  // F11键
#define KEY_F12 (0x58)  // F12键

#define KEY_SCROLL_LOCK (0x46) //
#define KEY_HOME (0x47)
#define KEY_END (0x4F)
#define KEY_PAGE_UP (0x49)      // PgUp键
#define KEY_PAGE_DOWN (0x51)    // PgDn键
#define KEY_CURSOR_UP (0x48)    // ⬆上移键
#define KEY_CURSOR_DOWN (0x50)  // ⬇下移键
#define KEY_CURSOR_RIGHT (0x4D) // ➡键
#define KEY_CURSOR_LEFT (0x4B)  // ⬅键
#define KEY_INSERT (0x52)       // insert键
#define KEY_DELETE (0x53)       // delete键
#define KEY_BACKSPACE 0xE       // BackSpace退格键
/**
 * 键盘映射结构
 */
typedef struct key_map_t
{
    uint8_t normal; // 普通键
    uint8_t func;   // 功能键
} key_map_t;
/**
 * 键盘状态
 */
typedef struct _keyboard_state_t
{
    int caps_lock : 1;    // 是否为大写
    int lshift_press : 1; // 左shift按下
    int rshift_press : 1; // 右shift按下
    int lalt_press : 1;   // 左alt键
    int ralt_press : 1;   // 右alt键
    int lctrl_press : 1;  // 左ctrl键
    int rctrl_press : 1;  // 右ctrl键
} kbd_state_t;

/**
 * @brief        : 初始化
 * @return        {*}
 **/
void kbd_init(void);
/**
 * @brief        : 中断处理函数
 * @return        {*}
 **/
void exception_handler_kbd(void);
/**
 * @brief        : 中断服务子程序
 * @param         {exception_frame_t *} frame: 函数栈帧
 * @return        {void}
 **/
void do_handler_kbd(exception_frame_t *frame);
#endif