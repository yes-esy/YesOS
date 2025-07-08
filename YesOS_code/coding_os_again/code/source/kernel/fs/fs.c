/**
 * @FilePath     : /code/source/kernel/fs/fs.c
 * @Description  :  文件系统实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-08 15:47:37
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "fs/fs.h"
#include "tools/klib.h"
#include "comm/boot_info.h"
#include "comm/cpu_instr.h"
#include "tools/log.h"
#include "dev/console.h"
#include "fs/file.h"
#include "dev/dev.h"
#include "core/task.h"
#include "cpu/irq.h"
#define TEMP_FILE_ID 100
#define TEMP_ADDR (8 * 1024 * 1024) // 在0x800000处缓存原始
static uint8_t *temp_pos;             // 文件读写位置
/**
 * @brief        : 使用LBA48模式读取磁盘
 * @param         {uint32_t} sector: 扇区号
 * @param         {uint32_t} sector_count: 扇区数量
 * @param         {uint8_t *} buff: 输入缓冲区
 * @return        {*} 程序入口地址
 **/
static void read_disk(uint32_t sector, uint32_t sector_count, uint8_t *buf)
{
    irq_state_t irq_state = irq_enter_protection();
    outb(0x1F6, (uint8_t)(0xE0));

    outb(0x1F2, (uint8_t)(sector_count >> 8));
    outb(0x1F3, (uint8_t)(sector >> 24)); // LBA参数的24 ~ 31位
    outb(0x1F4, (uint8_t)(0));            // LBA参数的32~39位
    outb(0x1F5, (uint8_t)(0));            // LBA参数的40~47位

    outb(0x1F2, (uint8_t)(sector_count));
    outb(0x1F3, (uint8_t)(sector));       // LBA参数的0~7位
    outb(0x1F4, (uint8_t)(sector >> 8));  // LBA参数的8~15位
    outb(0x1F5, (uint8_t)(sector >> 16)); // LBA参数的16~23位

    outb(0x1F7, (uint8_t)0x24);

    // 读取数据
    uint16_t *data_buf = (uint16_t *)buf;
    while (sector_count-- > 0)
    {
        // 每次扇区读之前都要检查,等待数据就绪
        while ((inb(0x1F7) & 0x88) != 0x8)
        {
        }

        // 读取数据并将数据写入到缓存中
        for (int i = 0; i < SECTOR_SIZE / 2; i++)
        {
            *data_buf++ = inw(0x1F0);
        }
    }
    irq_leave_protection(irq_state);
}
/**
 * @brief        : 判断文件路径是否有效
 * @param         {char *} path: 文件路径指针
 * @return        {int} 若有效返回1,无效返回0
 **/
static int is_path_valid(const char *path)
{
    if ((path == (const char *)0) || (path[0] == '\0')) // 路径为空
    {
        return 0; // 无效
    }
    return 1; // 有效
}

/**
 * @brief        : 打开文件
 * @param         {char} *path: 文件路径
 * @param         {int} flags: 标志
 * @return        {int} : 若成功返回文件描述符,失败返回-1
 **/
int sys_open(const char *path, int flags, ...)
{
    if (kernel_strncmp(path, "tty", 3) == 0) // 是否为tty设备
    {
        if (!is_path_valid(path))
        {
            log_printf("path is not valid.\n");
            return -1;
        }
        int fd = -1;                 // 记录文件描述符在进程文件表中对应的索引
        file_t *file = file_alloc(); // 分配一个文件描述符
        if (file != (file_t *)0)     // 成功分配
        {
            fd = task_alloc_fd(file); // 添加该文件到进程文件表中
            if (fd < 0)               // 添加失败
            {
                goto sys_open_failed; // 错误处理
            }
        }
        else
        {
            goto sys_open_failed;
        }
        if (kernel_strlen(path) < 5) // 判断路径长度是否合法
        {
            goto sys_open_failed;
        }
        int num = path[4] - '0';                // 获取该tty设备的序号
        int dev_id = dev_open(DEV_TTY, num, 0); // 打开该设备
        if (dev_id < 0)                         // 大开失败
        {
            goto sys_open_failed;
        }
        file->dev_id = dev_id;                                                // 记录设备id
        file->mode = 0;                                                       // 打开模式
        file->pos = 0;                                                        // 文件读写位置
        file->ref = 1;                                                        // 打开次数
        file->type = FILE_TTY;                                                // 文件类型
        kernel_memcpy((void *)file->file_name, (void *)path, FILE_NAME_SIZE); // 设置文件名
        return fd;                                                            // 返回文件描述符在进程文件表中对应的索引
    sys_open_failed:
        if (file) // 文件已经打开
        {
            file_free(file); // 释放文件
        }
        if (fd >= 0)
        {
            task_remove_fd(fd); // 从进程文件表释放该文件
        }
        return -1;
    }
    else
    {
        if (path[0] == '/')
        {
            // log_printf("Opening file: %s\n", path);
            // 暂时直接从扇区5000上读取, 读取大概40KB，足够了
            read_disk(5000, 80, (uint8_t *)TEMP_ADDR);
            temp_pos = (uint8_t *)TEMP_ADDR;
            return TEMP_FILE_ID;
        }
    }
    return -1;
}
/**
 * @brief        : 读取文件
 * @param         {int} file: 哪一个文件
 * @param         {char} *ptr: 读取到文件的目的地址
 * @param         {int} len: 读取长度
 * @return        {int} 读取成功返回读取长度,失败返回-1
 **/
int sys_read(int file, char *ptr, int len)
{
    if (file == TEMP_FILE_ID)
    {
        kernel_memcpy(ptr, temp_pos, len);
        temp_pos += len;
        return len;
    }
    else
    {
        file_t *cur_file = task_file(file); // 获取当前文件在进程中的文件描述符
        if (cur_file == (file_t *)0)        // 获取为空
        {
            log_printf("file not opened , file is %d\n", file);
            return -1;
        }
        return dev_read(cur_file->dev_id, 0, ptr, len); // 调用设备读取数据接口
    }
    return -1;
}
/**
 * @brief        : 写文件
 * @param         {int} file: 写入的文件
 * @param         {char} *ptr: 写入的内容的起始地址
 * @param         {int} len: 写入长度
 * @return        {int} : 若成功返回写入长度,失败返回-1
 **/
int sys_write(int file, char *ptr, int len)
{
    // if (file == 1)
    // {
    //     // ptr[len] = '\0';
    //     // log_printf("%s", ptr);
    //     // console_write(0, ptr, len);
    //     char buf[128];
    //     kernel_memcpy(buf, ptr, len);
    //     buf[len] = '\0';
    //     log_printf("%s", buf);
    // }
    // file = 0;
    file_t *cur_file = task_file(file); // 获取当前文件在进程中的文件描述符
    if (cur_file == (file_t *)0)        // 获取为空
    {
        log_printf("file not opened , file is %d\n", file);
        return -1;
    }
    return dev_write(cur_file->dev_id, 0, ptr, len); // 调用设备写接口
}
/**
 * @brief        : 调整读写指针
 * @param         {int} file: 操作文件
 * @param         {int} ptr: 要调整的指针
 * @param         {int} dir: 文件所在目录
 * @return        {int} 成功返回 0, 失败返回 -1
 **/
int sys_lseek(int file, int ptr, int dir)
{
    if (file == TEMP_FILE_ID)
    {
        temp_pos = (uint8_t *)(TEMP_ADDR + ptr); // 调整指针
        return 0;
    }
    return -1;
}
/**
 * @brief        : 关闭某个打开的文件
 * @param         {int} file: 操作的文件
 * @return        {int} : 返回0
 **/
int sys_close(int file)
{
    return 0;
}
/**
 * @brief        : 判断文件是否为tty设备
 * @param         {int} file: 操作的文件
 * @return        {int} : 返回0
 **/
int sys_isatty(int file)
{
    return 0;
}
/**
 * @brief        :
 * @param         {stat} *st:
 * @return        {*}
 **/
int sys_fstat(struct stat *st)
{

    return 0;
}
/**
 * @brief        : 复制文件描述符
 * @param         {int} file: 要复制的文件描述符索引
 * @return        {int} 新的文件描述符，失败返回-1
 **/
int sys_dup(int file)
{
    if ((file < 0) || (file >= TASK_OPEN_FILE_NR)) // 文件描述符是否合法
    {
        log_printf("file %d id not valid.\n" ,file);
        return -1;
    }
    file_t *cur_file = task_file(file); // 当前文件描述符
    if (cur_file == (file_t *)0)        // 获取失败
    {
        log_printf("file not opened.\n");
        return -1;
    }
    int fd = task_alloc_fd(cur_file); // 分配一个新表项
    if (fd < 0)                       // 分配失败
    {
        log_printf("no task file available.\n");
        return -1;
    }
    cur_file->ref++; // 增加打开次数
    return fd;
}
/**
 * @brief        : 文件系统初始化
 * @return        {void}
 **/
void fs_init(void)
{
    file_table_init(); // 文件表初始化
}
