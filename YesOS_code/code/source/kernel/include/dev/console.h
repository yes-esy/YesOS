#ifndef CONSOLE_H
#define CONSOLE_H

#include "dev/tty.h"
#include "comm/types.h"
#include "ipc/mutex.h"
#define CONSOLE_DISP_ADDR 0xB8000
#define CONSOLE_DISP_END (0xB8000 + 32 * 1024)
#define CONSOLE_ROW_MAX 25
#define CONSOLE_COL_MAX 80
#define ESC_PARAM_MAX 10

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

typedef union _display_char_t
{
    struct
    {
        char c;
        char foregroud : 4;
        char background : 3;
    };
    uint16_t v;
} display_char_t;

typedef struct _console_t
{
    enum
    {
        CONSOLE_WRITE_NORMAL,
        CONSOLE_WRITE_ESC,
        CONSOLE_WRITE_SQUARE,
    } write_state;
    display_char_t *disp_base;
    int cursor_row, cursor_col;
    int display_rows, display_cols;
    cclor_t foregroud, background;

    int esc_param[ESC_PARAM_MAX];
    int esc_param_index;

    int old_cursor_row, old_cursor_col;
    mutex_t mutex;
} console_t;

int console_init(int idx);

int console_write(tty_t *tty);

void console_close(int console);

int console_select(int index);

#endif