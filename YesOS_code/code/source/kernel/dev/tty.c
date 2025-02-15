#include "dev/tty.h"
#include "dev/dev.h"
#include "tools/log.h"
#include "dev/kbd.h"
#include "dev/console.h"
#include "cpu/irq.h"

static tty_t tty_devs[TTY_NR];
static int curr_tty = 0;

/**
 * @brief 判断tty是否有效
 */
static inline tty_t *get_tty(device_t *dev)
{
    int tty = dev->minor;
    if ((tty < 0) || (tty >= TTY_NR) || (!dev->open_count))
    {
        log_printf("tty is not opened. tty = %d", tty);
        return (tty_t *)0;
    }

    return tty_devs + tty;
}
int tty_write(device_t *dev, int addr, char *buf, int size)
{
    if (size < 0)
    {
        log_printf("tty_write: invalid size %d\n", size);
        return -1;
    }
    tty_t *tty = get_tty(dev);
    if (!tty)
    {
        return -1;
    }
    int len = 0;
    while (size)
    {
        char c = *buf++;
        if ((c == '\n') && (tty->oflags & TTY_OCRLF))
        {
            sem_wait(&tty->osem);
            int err = tty_fifo_put(&tty->ofifo, '\r');
            if (err < 0)
            {
                break;
            }
        }
        sem_wait(&tty->osem);
        int err = tty_fifo_put(&tty->ofifo, c);
        if (err < 0)
        {
            break;
        }
        len++;
        size--;
        // 显示器正在忙，空闲则发送
        console_write(tty);
    }
    return len;
}

void tty_fifo_init(tty_fifo_t *fifo, char *buf, int size)
{
    fifo->buf = buf;
    fifo->size = size;
    fifo->read = fifo->write = 0;
    fifo->count = 0;
}
int tty_open(device_t *dev)
{
    int idx = dev->minor;
    if (idx < 0 || idx >= TTY_NR)
    {
        log_printf("tty_open: invalid minor number %d\n", idx);
        return -1;
    }
    tty_t *tty = &tty_devs[idx];
    tty_fifo_init(&tty->ofifo, tty->obuf, TTY_OBUF_SIZE);
    sem_init(&tty->osem, TTY_OBUF_SIZE);
    tty_fifo_init(&tty->ififo, tty->ibuf, TTY_IFIFO_SIZE);
    sem_init(&tty->isem, 0);
    tty->console_idx = idx;
    tty->oflags = TTY_OCRLF;
    tty->iflags = TTY_INLCR | TTY_IECHO;

    kbd_init();
    console_init(idx);
    return 0;
}
int tty_read(device_t *dev, int addr, char *buf, int size)
{
    if (size < 0)
    {
        log_printf("tty_read: invalid size %d\n", size);
        return -1;
    }
    tty_t *tty = get_tty(dev);
    char *pbuf = buf;

    int len = 0;
    while (len < size)
    {
        sem_wait(&tty->isem);
        char c;
        tty_fifo_get(&tty->ififo, &c);

        switch (c)
        {
        case 0x7F:
            if (len == 0)
            {
                continue;
            }
            len--;
            pbuf--;
            break;
        case '\n':
            if (tty->iflags & TTY_INLCR && len < size - 1)
            {
                *pbuf++ = '\r';
                len++;
            }
            *pbuf++ = '\n';
            len++;
            break;
        default:
            *pbuf++ = c;
            len++;
            break;
        }
        if (tty->iflags & TTY_IECHO)
        {
            tty_write(dev, 0, &c, 1);
        }
        if ((c == '\n') || (c == '\r'))
        {
            break;
        }
    }
    return len;
}

/**
 * @brief 写一字节数据
 */
int tty_fifo_put(tty_fifo_t *fifo, char c)
{
    int state = irq_enter_protection();
    if (fifo->count >= fifo->size)
    {
        irq_leave_protection(state);
        return -1;
    }

    fifo->buf[fifo->write++] = c;
    if (fifo->write >= fifo->size)
    {
        fifo->write = 0;
    }
    fifo->count++;
    irq_leave_protection(state);
    return 0;
}
/**
 * @brief 取一字节数据
 */
int tty_fifo_get(tty_fifo_t *fifo, char *c)
{
    int state = irq_enter_protection();
    if (fifo->count <= 0)
    {
        irq_leave_protection(state);
        return -1;
    }

    *c = fifo->buf[fifo->read++];
    if (fifo->read >= fifo->size)
    {
        fifo->read = 0;
    }
    fifo->count--;
    irq_leave_protection(state);
    return 0;
}

int tty_control(device_t *dev, int cmd, int arg0, int arg1)
{
    return 0;
}
void tty_close(device_t *dev)
{
    return;
}

void tty_in(char key)
{
    tty_t *tty = tty_devs + curr_tty;
    if (sem_count(&tty->isem) >= TTY_IFIFO_SIZE)
    {
        return;
    }
    tty_fifo_put(&tty->ififo, key);
    sem_signal(&tty->isem);
}

/**
 * @brief 选择tty
 */
void tty_select(int index)
{
    if (index != curr_tty)
    {
        console_select(index);
        curr_tty = index;
    }
}

dev_desc_t dec_tty_desc = {
    .name = "tty",
    .major = DEV_TTY,
    .open = tty_open,
    .read = tty_read,
    .write = tty_write,
    .control = tty_control,
    .close = tty_close,
};