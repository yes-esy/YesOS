/**
 * @FilePath     : /code/source/kernel/dev/disk.c
 * @Description  :  磁盘.c实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-10 13:20:41
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "dev/disk.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "comm/cpu_instr.h"
#include "comm/boot_info.h"
static disk_t disk_buf[DISK_CNT];

/**
 * @brief        : 向磁盘发命令
 * @param         {disk_t *} disk: 目的磁盘
 * @param         {uint32_t} start_sector: 起始扇区
 * @param         {uint32_t} sector_count: 扇区数量
 * @param         {int} cmd: 所发送的命令
 * @return        {void}
 **/
static void disk_send_cmd(disk_t *disk, uint32_t start_sector, uint32_t sector_count, int cmd)
{
    outb(DISK_DRIVE(disk), (DISK_DRIVE_BASE | disk->drive));     // 设置驱动器和LBA模式，bit4选择驱动器(0=主盘,1=从盘)，bit6=1启用LBA模式
    outb(DISK_SECTOR_COUNT(disk), (uint8_t)(sector_count >> 8)); // 设置扇区计数高8位
    outb(DISK_LBA_LOW(disk), (uint8_t)(start_sector >> 24));     // 设置LBA地址的24-31位
    outb(DISK_LBA_MID(disk), 0);                                 // 设置LBA地址的32-39位
    outb(DISK_LBA_HI(disk), 0);                                  // 设置LBA地址的40-47位
    outb(DISK_SECTOR_COUNT(disk), (uint8_t)(sector_count));      // 设置扇区计数低8位
    outb(DISK_LBA_LOW(disk), (uint8_t)(start_sector >> 0));      // 设置LBA地址的0-7位
    outb(DISK_LBA_MID(disk), (uint8_t)(start_sector >> 8));      // 设置LBA地址的8-15位
    outb(DISK_LBA_HI(disk), (uint8_t)(start_sector >> 16));      // 设置LBA地址的16-23位
    outb(DISK_CMD(disk), (uint8_t)cmd);                          // 发送命令到命令寄存器
}
/**
 * @brief        : 读取磁盘的数据到缓冲区
 * @param         {disk_t *} disk: 需要读取的磁盘
 * @param         {void *} buf: 读取到的数据的缓冲区
 * @param         {int} size: 读取的大小
 * @return        {void}
 **/
static void disk_read_data(disk_t *disk, void *buf, int size)
{
    uint16_t *p = (uint16_t *)buf; // 指向缓冲区首地址

    for (int i = 0; i < size / 2; i++)
    {
        *p++ = inw(DISK_DATA(disk));
    }
}
/**
 * @brief        : 往磁盘写入数据
 * @param         {disk_t *} disk: 需要写入的磁盘
 * @param         {void *} buf: 写入的数据存储的缓冲区
 * @param         {int} size: 写入的大小
 * @return        {void}
 **/
static inline void disk_write_data(disk_t *disk, void *buf, int size)
{
    uint16_t *p = (uint16_t *)buf; // 写入数据缓冲区首地址
    for (int i = 0; i < size / 2; i++)
    {
        outw(DISK_DATA(disk), *p++);
    }
}
/**
 * @brief        : 等待磁盘(磁盘存在)状态空闲
 * @param         {disk_t} *disk: 等待的磁盘
 * @return        {int} 若磁盘空闲返回0,否则(非空闲或错误)返回-1
 **/
static int disk_wait_data(disk_t *disk)
{
    uint8_t status; // 记录状态
    do
    {
        // 等待数据或者有错误
        status = inb(DISK_STATUS(disk)); // 读取磁盘状态
        if ((status &
             (DISK_STATUS_BUSY | DISK_STATUS_DRQ | DISK_STATUS_ERR)) != DISK_STATUS_BUSY) // 磁盘空闲
        {
            break;
        }
    } while (1);

    return (status & DISK_STATUS_ERR) ? -1 : 0; // 磁盘是否空闲
}
/**
 * @brief        : 打印磁盘信息
 * @param         {disk_t *} : 需要打印的磁盘
 * @return        {void}
 **/
static void print_disk_info(disk_t *disk)
{
    log_printf("disk : %s\n", disk->name);                                                  // 打印磁盘名称
    log_printf("port base:%x\n", disk->port_base);                                         // 端口地址
    log_printf("total size %d m\n", disk->sector_count * disk->sector_size / 1024 / 1024); // 磁盘大小
}

/**
 * @brief        : 识别磁盘
 * @param         {disk_t} *disk: 需要识别的磁盘
 * @return        {int} 若成功返回0,失败返回-1
 **/
static int identify_disk(disk_t *disk)
{
    disk_send_cmd(disk, 0, 0, DISK_CMD_IDENTIFY); // 发送识别命令
    int err = inb(DISK_STATUS(disk));             // 读取磁盘状态
    if (err == 0)                                 // 不存在
    {
        log_printf("%s dosen't exist\n", disk->name);
        return -1;
    }
    // 存在
    err = disk_wait_data(disk); // 读取磁盘状态
    if (err < 0)                // 非空闲
    {
        log_printf("disk[%s]:read faied.\n", disk->name);
        return -1;
    }
    uint16_t buf[256];
    disk_read_data(disk, buf, sizeof(buf)); // 读取磁盘
    disk->sector_count = *(uint32_t *)(buf + 100);
    disk->sector_size = SECTOR_SIZE;
    return 0;
}
/**
 * @brief        : 磁盘初始化
 * @return        {void}
 **/
void disk_init(void)
{
    log_printf("check disk...\n");
    kernel_memset((void *)disk_buf, 0, sizeof(disk_buf));
    for (int i = 0; i < DISK_PER_CHANNEL; i++)
    {
        disk_t *disk = disk_buf + i;                       // 获取该硬盘
        kernel_sprintf(disk->name, "sd%c", i + 'a');       // 对磁盘命名
        disk->drive = (i == 0) ? DISK_MASTER : DISK_SLAVE; // 主盘还是从盘
        disk->port_base = IOBASE_PRIMARY;                  // 设置端口地址
        int err = identify_disk(disk);                     // 识别磁盘
        if (err == 0)                                      // 识别错误
        {
            print_disk_info(disk);
        }
    }
}