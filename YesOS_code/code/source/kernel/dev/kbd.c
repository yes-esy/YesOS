#include "dev/kbd.h"
#include "cpu/irq.h"
#include "tools/log.h"
#include "comm/cpu_instr.h"
#include "comm/types.h"
#include "tools/klib.h"
#include "dev/tty.h"
static kbd_state_t kbd_state; // 键盘状态
static const key_map_t map_table[256] = {
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
    [0x0E] = {ASCII_DEL, ASCII_DEL},
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
};

static void do_fx_key(int key)
{
    int index = key - KEY_F1;
    if(kbd_state.lctrl_press || kbd_state.rctrl_press)
    {
        tty_select(index);
    }
}

void kbd_init(void)
{
    static int inited = 0;
    if (!inited)
    {
        kernel_memset((void *)&kbd_state, 0, sizeof(kbd_state_t));
        irq_install(IRQ1_KEYBOARD, (irq_handler_t)exception_handler_kbd);
        irq_enable(IRQ1_KEYBOARD);
        inited = 1;
    }
    return;
}

static inline int is_make_code(uint8_t raw_code)
{
    return !(raw_code & 0x80);
}

static inline char get_key(uint8_t key_code)
{
    return key_code & 0x7F;
}

static void do_normal_key(uint8_t raw_code)
{
    char key = get_key(raw_code);         // 获取键值
    int is_make = is_make_code(raw_code); // 获取按键状态

    switch (key)
    {
    case KEY_LSHIFT:
        kbd_state.lshift_press = is_make ? 1 : 0;
        break;
    case KEY_RSHIFT:
        kbd_state.rshift_press = is_make ? 1 : 0;
        break;
    case KEY_CAPS:
        if (is_make)
            kbd_state.caps_lock = ~kbd_state.caps_lock;
        break;
    case KEY_ALT:
        kbd_state.lalt_press = is_make;
        break;
    case KEY_CTRL:
        kbd_state.lctrl_press = is_make;
        break;
    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
    case KEY_F5:
    case KEY_F6:
    case KEY_F7:
    case KEY_F8:
    case KEY_F9:
        do_fx_key(key);
        break;
    case KEY_F10:
    case KEY_F11:
    case KEY_F12:
        break;
        break;
    default:
        if (is_make)
        {
            if (kbd_state.lshift_press || kbd_state.rshift_press)
            {
                key = map_table[key].func; // 获取附加功能键值
                log_printf("kbd raw code: %c\n", key);
            }
            else
            {
                key = map_table[key].normal;
            }
            if (kbd_state.caps_lock)
            {
                if (key >= 'a' && key <= 'z')
                {
                    key = key - 'a' + 'A';
                }
                else if (key >= 'A' && key <= 'Z')
                {
                    key = key - 'A' + 'a';
                }
            }
            // log_printf("kbd raw code: %c\n", key);
            tty_in(key);
        }
        break;
    }
}

static void do_e0_key(int raw_code)
{
    char key = get_key(raw_code);         // 获取键值
    int is_make = is_make_code(raw_code); // 获取按键状态

    switch (key)
    {
    case KEY_CTRL:
        kbd_state.rctrl_press = is_make;
        break;
    case KEY_ALT:
        kbd_state.ralt_press = is_make;
        break;
    }
}

void do_handler_kbd(exception_frame_t *frame)
{
    static enum {
        NORMAL,
        BEGIN_E0,
        BEGIN_E1,
    } recv_state = NORMAL;
    // 检查是否有数据，无数据则退出
    uint8_t status = inb(KBD_PORT_STAT);
    if (!(status & KBD_STAT_RECV_READY))
    {
        pic_send_eoi(IRQ1_KEYBOARD);
        return;
    }
    uint8_t raw_code = inb(KBD_PORT_DATA);
    pic_send_eoi(IRQ1_KEYBOARD);

    if (raw_code == KEY_E0)
    {
        recv_state = BEGIN_E0;
    }
    else if (raw_code == KEY_E1)
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
        case BEGIN_E0:
            do_e0_key(raw_code);
            recv_state = NORMAL;
            break;
        case BEGIN_E1:
            recv_state = NORMAL;
            break;
        }
    }
}