/**
 * @FilePath     : /code/source/kernel/dev/disk.c
 * @Description  :  磁盘.c实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-12 15:13:45
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "dev/disk.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "comm/cpu_instr.h"
#include "comm/boot_info.h"
#include "dev/dev.h"
#include "cpu/irq.h"
static disk_t disk_buf[DISK_CNT]; // 磁盘
static sem_t op_sem;              // 操作信号量
static mutex_t mutex;             // 互斥锁
static int task_on_op;            // 标志位,表明进程在操作磁盘
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
    log_printf("disk : %s\n", disk->name);                                                 // 打印磁盘名称
    log_printf("port base:%x\n", disk->port_base);                                         // 端口地址
    log_printf("total size %d m\n", disk->sector_count * disk->sector_size / 1024 / 1024); // 磁盘大小
    for (int i = 0; i < DISK_PRIMARY_PART_CNT; i++)
    {
        part_info_t *part_info = disk->part_info + i;

        if (part_info->type == FS_INVALID)
        {
            continue;
        }
        log_printf("%s: \n type:%x, start sector:%d, count:%d\n",
                   part_info->name,
                   part_info->type,
                   part_info->start_sector,
                   part_info->total_sector);
    }
}
/**
 * @brief        : 检测磁盘分区信息
 * @param         {disk_t} *disk: 需要检测的磁盘
 * @return        {int} : 成功返回0,失败返回-1
 **/
static int detect_part_info(disk_t *disk)
{
    mbr_t mbr;
    disk_send_cmd(disk, 0, 1, DISK_CMD_READ); // 发送读命令
    int err = disk_wait_data(disk);           // 等待磁盘状态
    if (err < 0)                              // 读取失败
    {
        log_printf("read mbr failed.\n");
        return err;
    }
    disk_read_data(disk, &mbr, sizeof(mbr)); // 读取数据到mbr中
    part_item_t *item = mbr.part_item;
    part_info_t *part_info = disk->part_info + 1; // 第一个分区信息
    for (int i = 0; i < MBR_PRIMARY_PART_NR; i++, item++, part_info++)
    {
        part_info->type = item->system_id; // 设置类型
        if (part_info->type == FS_INVALID) // 分区无效
        {
            part_info->total_sector = 0;
            part_info->start_sector = 0;
            part_info->disk = (disk_t *)0;
        }
        else // 分区有效
        {
            kernel_sprintf(part_info->name, "%s%d", disk->name, i + 1); // 设置分区名
            part_info->start_sector = item->relative_sector;            // 起始扇区号
            part_info->total_sector = item->total_sectors;              // 磁盘扇区数量
            part_info->disk = disk;                                     // 将分区信息与它所属的磁盘关联起来。
        }
    }
    return 0;
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
    // 第0个分区描述整个磁盘的情况
    part_info_t *part = disk->part_info + 0;           // 第0项
    part->disk = disk;                                 // 设置磁盘
    kernel_sprintf(part->name, "%s%d", disk->name, 0); // 设置分区名
    part->start_sector = 0;                            // 起始扇区号
    part->total_sector = disk->sector_count;           // 磁盘扇区数量
    part->type = FS_INVALID;                           // 分区类型未知
    detect_part_info(disk);                            // 解析分区信息
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
    mutex_init(&mutex);
    sem_init(&op_sem, 0);
    for (int i = 0; i < DISK_PER_CHANNEL; i++)
    {
        disk_t *disk = disk_buf + i;                       // 获取该硬盘
        kernel_sprintf(disk->name, "sd%c", i + 'a');       // 对磁盘命名
        disk->drive = (i == 0) ? DISK_MASTER : DISK_SLAVE; // 主盘还是从盘
        disk->port_base = IOBASE_PRIMARY;                  // 设置端口地址
        disk->mutex = &mutex;                              //
        disk->op_sem = &op_sem;
        int err = identify_disk(disk); // 识别磁盘
        if (err == 0)                  // 识别错误
        {
            print_disk_info(disk);
        }
    }
}
void do_handler_ide_primary(exception_frame_t *frame)
{
    pic_send_eoi(IRQ14_HARD_DISK_PRIMARY);
    if (task_on_op && task_current()) // 有进程且有进程操作磁盘
    {
        sem_signal(&op_sem); // 发信号通知
    }
}
/**
 * @brief        : 打开磁盘的函数接口
 * @param         {device_t *} dev: 打开的设备
 * @return        {int} : 若成功打开返回0，否则返回-1
 **/
int disk_open(device_t *dev)
{
    // 0xa0 -- a磁盘编号a,b,c,d 分区号0,1,2
    int disk_index = (dev->minor >> 4) - 0xA;                              // 磁盘索引
    int part_index = ((dev->minor) & 0xF);                                 // 分区索引
    if ((disk_index >= DISK_CNT) || (part_index >= DISK_PRIMARY_PART_CNT)) // 磁盘或分区索引不合法
    {
        log_printf("disk device minor error: %d\n", dev->minor);
        return -1;
    }
    disk_t *disk = disk_buf + disk_index; // 取出磁盘结构
    if (disk->sector_count == 0)          // 磁盘不存在
    {
        log_printf("disk not exist , disk device sd%x.\n", dev->minor);
        return -1;
    }
    part_info_t *part_info = disk->part_info + part_index; // 取出分区信息
    if (part_info->total_sector == 0)
    {
        log_printf("part not exist, part index : sd%x", dev->minor);
        return -1;
    }
    dev->data = part_info;                                                              // 临时保存分区信息；
    irq_install(IRQ14_HARD_DISK_PRIMARY, (irq_handler_t)exception_handler_ide_primary); // 安装中断处理程序
    irq_enable(IRQ14_HARD_DISK_PRIMARY);                                                // 开启中断
    return 0;
}
/**
 * @brief        : 读取磁盘的数据
 * @param         {device_t} *dev: 具体的磁盘的指针
 * @param         {int} start_sector: 从磁盘的开始扇区
 * @param         {char *} buf: 读取到哪里
 * @param         {int} sector_counts: 读取的扇区数量
 * @return        {int} : 成功返回0,失败返回-1
 **/
int disk_read(device_t *dev, int start_sector, char *buf, int sector_counts)
{
    part_info_t *part_info = (part_info_t *)dev->data;
    if ((part_info_t *)0 == part_info)
    {
        log_printf("Get part info failed.device :%d\n", dev->minor);
        return -1;
    }
    disk_t *disk = part_info->disk; // 获取磁盘结构
    if ((disk_t *)0 == disk)        // 无效磁盘
    {
        log_printf("No disk. device %d\n", dev->minor);
        return -1;
    }
    mutex_lock(disk->mutex); // 上锁
    task_on_op = 1;

    int cnt;
    disk_send_cmd(disk, part_info->start_sector + start_sector, sector_counts, DISK_CMD_READ); // 发送读取数据命令
    for (cnt = 0; cnt < sector_counts; cnt++, buf += disk->sector_size)
    {
        if (task_current()) // 有进程
        {
            sem_wait(disk->op_sem); // 等信号
        }
        int err = disk_wait_data(disk); //
        if (err < 0)
        {
            log_printf("disk(%s) read error: start sector %d, count : %d", disk->name, start_sector, sector_counts);
            break;
        }
        disk_read_data(disk, buf, disk->sector_size); // 读取数据
    }
    mutex_unlock(disk->mutex);
    return cnt;
}
/**
 * @brief        : 往磁盘写入数据
 * @param         {device_t} *dev: 写入数据的磁盘
 * @param         {int} addr: 从磁盘的哪里开始写
 * @param         {char} *buf: 写入的数据的起始地址
 * @param         {int} size: 写入的数据量
 * @return        {int} : 返回0
 **/
int disk_write(device_t *dev, int start_sector, char *buf, int sector_counts)
{
    part_info_t *part_info = (part_info_t *)dev->data;
    if ((part_info_t *)0 == part_info)
    {
        log_printf("Get part info failed.device :%d\n", dev->minor);
        return -1;
    }
    disk_t *disk = part_info->disk; // 获取磁盘结构
    if ((disk_t *)0 == disk)        // 无效磁盘
    {
        log_printf("No disk. device %d\n", dev->minor);
        return -1;
    }
    mutex_lock(disk->mutex); // 上锁
    task_on_op = 1;
    disk_send_cmd(disk, part_info->start_sector + start_sector, sector_counts, DISK_CMD_WRITE); // 发送读取数据命令
    int cnt;
    for (cnt = 0; cnt < sector_counts; cnt++, buf += disk->sector_size)
    {
        disk_write_data(disk, buf, disk->sector_size); // 写入数据

        if (task_current()) // 有进程才等待
        {
            sem_wait(disk->op_sem); // 等信号
        }
        int err = disk_wait_data(disk); // 等待磁盘状态
        if (err < 0)
        {
            log_printf("disk(%s) read error: start sector %d, count : %d", disk->name, start_sector, sector_counts);
            break;
        }
    }
    mutex_unlock(disk->mutex);
    return cnt;
}
/**
 * @brief        : 控制磁盘
 * @param         {device_t *} dev: 具体的磁盘描述指针
 * @param         {int} cmd: 控制命令
 * @param         {int} arg0: 命令的参数1
 * @param         {int} arg1: 命令的参数2
 * @return        {int} : 返回0
 **/
int disk_control(device_t *dev, int cmd, int arg0, int arg1)
{
}
/**
 * @brief        : 关闭磁盘
 * @param         {device_t *} dev: 具体的设备描述的指针
 * @return        {void}
 **/
void disk_close(device_t *dev)
{
}

/**
 * disk设备描述
 */
dev_desc_t dev_disk_desc = {
    .name = "disk_device",
    .major = DEV_DISK,
    .open = disk_open,
    .read = disk_read,
    .write = disk_write,
    .control = disk_control,
    .close = disk_close,
};