#ifndef TTY_H
#define TTY_H
#define TTY_OBUF_SIZE 512
#define TTY_IFIFO_SIZE 512
#define TTY_NR 8
#define TTY_OCRLF (1 << 0)
#define TTY_IECHO (1 << 2)
#define TTY_INLCR (1 << 0) // 将\n转成\r\n
#include "ipc/sem.h"
typedef struct _tty_fifo_t
{
    char *buf;
    int size;
    int read, write;
    int count;
} tty_fifo_t;

typedef struct _tty_t
{
    char obuf[TTY_OBUF_SIZE]; //输出缓冲区
    tty_fifo_t ofifo;
    sem_t osem;
    char ibuf[TTY_IFIFO_SIZE];
    tty_fifo_t ififo;
    sem_t isem; 
    int oflags;
    int iflags;
    int console_idx;
} tty_t;

int tty_fifo_get(tty_fifo_t *fifo, char *c);
int tty_fifo_put(tty_fifo_t *fifo, char c);
void tty_in(char key);
void tty_select(int index);
#endif