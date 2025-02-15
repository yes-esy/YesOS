#include "dev/disk.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "comm/cpu_instr.h"
#include "comm/types.h"
#include "comm/boot_info.h"
#include "dev/dev.h"
#include "cpu/irq.h"
static disk_t disk_buff[DISK_CNT];
static mutex_t mutex;
static sem_t op_sem;

static int task_on_op;
/**
 * @brief 发送命令
 */
static void disk_send_cmd(disk_t *disk, uint32_t start_sector, uint32_t sector_count, int cmd)
{
    outb(DISK_DRIVE(disk), DISK_DRIVE_BASE | disk->drive);
    // 必须先写高字节
    outb(DISK_SECTOR_COUNT(disk), (uint8_t)(sector_count >> 8)); // 扇区数高8位
    outb(DISK_LBA_LOW(disk), (uint8_t)(start_sector >> 24));     // LBA参数的24~31位
    outb(DISK_LBA_MID(disk), 0);                                 // 高于32位不支持
    outb(DISK_LBA_HIGH(disk), 0);                                // 高于32位不支持
    outb(DISK_SECTOR_COUNT(disk), (uint8_t)(sector_count));      // 扇区数量低8位
    outb(DISK_LBA_LOW(disk), (uint8_t)(start_sector >> 0));      // LBA参数的0-7
    outb(DISK_LBA_MID(disk), (uint8_t)(start_sector >> 8));      // LBA参数的8-15位
    outb(DISK_LBA_HIGH(disk), (uint8_t)(start_sector >> 16));    // LBA参数的16-23位

    // 选择对应的主-从磁盘
    outb(DISK_CMD(disk), (uint8_t)cmd);
}

/**
 * @brief 读取数据
 */
static void disk_read_data(disk_t *disk, void *buff, int count)
{
    uint16_t *c = (uint16_t *)buff;
    for (int i = 0; i < count / 2; i++)
    {
        *c++ = inw(DISK_DATA(disk));
    }
}

/**
 * @brief 写入数据
 */
static void disk_write_data(disk_t *disk, void *buff, int count)
{
    uint16_t *c = (uint16_t *)buff;
    for (int i = 0; i < count / 2; i++)
    {
        outw(DISK_DATA(disk), *c++);
    }
}

/**
 * @brief 等待磁盘准备好数据
 */
static int disk_wait_data(disk_t *disk)
{
    uint8_t status;
    do
    {
        // 等待数据或者有错误
        status = inb(DISK_STATUS(disk));
        if ((status & (DISK_STATUS_BUSY | DISK_STATUS_DRQ | DISK_STATUS_ERR)) != DISK_STATUS_BUSY)
        {
            break;
        }
    } while (1);

    // 检查是否有错误
    return (status & DISK_STATUS_ERR) ? -1 : 0;
}

/**
 * @brief 打印磁盘信息
 */
static void print_info_disk(disk_t *disk)
{
    log_printf("%s:", disk->name);
    log_printf("  port_base: %x", disk->port_base);
    log_printf("  total_size: %d m", disk->sector_count * disk->sector_size / 1024 / 1024);
    log_printf("  drive: %s", disk->drive == DISK_MASTER ? "Master" : "Slave");

    // 显示分区信息
    log_printf("  Part info:");
    for (int i = 0; i < DISK_PRIMARY_PART_CNT; i++)
    {
        partinfo_t *part_info = disk->partinfo + i;
        if (part_info->type != FS_INVALID)
        {
            log_printf("    %s: type: %x, start sector: %d, count %d",
                       part_info->name, part_info->type,
                       part_info->start_sector, part_info->total_sector);
        }
    }
}
/**
 * @brief 识别分区信息
 * @param disk 磁盘
 */
static int detect_part_info(disk_t *disk)
{
    mbr_t mbr;
    disk_send_cmd(disk, 0, 1, DISK_CMD_READ);
    int err = disk_wait_data(disk);
    if (err < 0)
    {
        log_printf("disk[%s] wait data error", disk->name);
        return -1;
    }
    disk_read_data(disk, &mbr, sizeof(mbr));
    part_item_t *item = mbr.part_item;
    partinfo_t *part_info = disk->partinfo + 1;
    for (int i = 0; i < MBR_PRIMARY_PART_NR; i++, item++, part_info++)
    {
        part_info->type = item->system_id;
        if (part_info->type == FS_INVALID)
        {
            part_info->total_sector = 0;
            part_info->start_sector = 0;
            part_info->disk = (disk_t *)0;
        }
        else
        {
            kernel_sprintf(part_info->name, "%s%d", disk->name, i + 1);
            part_info->start_sector = item->relative_sectors;
            part_info->total_sector = item->total_sectors;
            part_info->disk = disk;
        }
    }
}
/**
 * @brief 识别磁盘
 */
static int
identify_disk(disk_t *disk)
{
    // 发送识别磁盘命令
    disk_send_cmd(disk, 0, 0, DISK_CMD_IDENTIFY);

    int err = inb(DISK_STATUS(disk));
    if (err == 0)
    {
        log_printf("%s disk not found", disk->name);
        return -1;
    }

    // 等待磁盘准备好数据
    err = disk_wait_data(disk);
    if (err < 0)
    {
        log_printf("disk[%s] wait data error", disk->name);
        return err;
    }

    uint16_t buf[256];
    // 读取数据
    disk_read_data(disk, buf, sizeof(buf));
    disk->sector_count = *((uint32_t *)(buf + 100));
    disk->sector_size = SECTOR_SIZE;

    partinfo_t *part_info = disk->partinfo + 0;
    part_info->disk = disk;
    kernel_sprintf(part_info->name, "%s%d", disk->name, 0);
    part_info->start_sector = 0;
    part_info->total_sector = disk->sector_count;
    part_info->type = FS_INVALID;

    detect_part_info(disk);
    return 0;
}

void disk_init(void)
{
    log_printf("disk check");

    kernel_memset((void *)disk_buff, 0, sizeof(disk_buff));
    mutex_init(&mutex);
    sem_init(&op_sem, 0);
    for (int i = 0; i < DISK_PER_CHANNEL; i++)
    {
        disk_t *disk = disk_buff + i;

        // sd开头
        kernel_sprintf(disk->name, "sd%c", i + 'a');

        disk->drive = (i == 0) ? DISK_MASTER : DISK_SLAVE;

        disk->mutex = &mutex;
        disk->op_sem = &op_sem;
        disk->port_base = IOBASE_PRIMARY;
        int err = identify_disk(disk);
        if (err == 0)
        {
            print_info_disk(disk);
        }
    }
}

/**
 * @brief 打开磁盘
 */
int disk_open(device_t *dev)
{
    // 0xa0 a-->磁盘编号 0 --> 分区号
    int disk_idx = (dev->minor >> 4) - 0xa;
    int part_idx = dev->minor & 0xF;

    if ((disk_idx >= DISK_CNT) || (part_idx >= DISK_PRIMARY_PART_CNT))
    {
        log_printf("disk open error: disk_idx: %d, part_idx: %d", disk_idx, part_idx);
        return -1;
    }
    disk_t *disk = disk_buff + disk_idx;

    if (disk->sector_size == 0)
    {
        log_printf("disk[%s] not exist", disk->name);
        return -1;
    }

    partinfo_t *part_info = disk->partinfo + part_idx;
    if (part_info->total_sector == 0)
    {
        log_printf("disk[%s] part[%d] not exist", disk->name, part_idx);
        return -1;
    }

    dev->data = part_info;
    irq_install(IRQ14_HARDDISK_PRIMARY, exception_handler_ide_primary);
    irq_enable(IRQ14_HARDDISK_PRIMARY);
    return 0;
}
/**
 * @brief 读取磁盘
 */
int disk_read(device_t *dev, int addr, char *buf, int size)
{
    partinfo_t *part_info = (partinfo_t *)dev->data;
    if (!part_info)
    {
        log_printf("disk read error, part_info is null device: %d", dev->minor);
        return -1;
    }
    disk_t *disk = part_info->disk;
    if (disk == (disk_t *)0)
    {
        log_printf("disk read error, disk is null device: %d", dev->minor);
        return -1;
    }
    mutex_lock(disk->mutex);
    task_on_op = 1;

    int cnt;
    disk_send_cmd(disk, part_info->start_sector + addr, size, DISK_CMD_READ);
    for (cnt = 0; cnt < size; cnt ++,buf += disk->sector_size)
    {
        sem_wait(disk->op_sem);
        int err = disk_wait_data(disk);
        if (err < 0)
        {
            log_printf("disk[%s] wait data error,start sector:%d,count:%d", disk->name, addr, cnt);
            break;
        }
        disk_read_data(disk, buf, disk->sector_size);
    }
    mutex_unlock(disk->mutex);
    return cnt;
}

/**
 * @brief 写入磁盘
 */
int disk_write(device_t *dev, int addr, char *buf, int size)
{
    partinfo_t *part_info = (partinfo_t *)dev->data;
    if (!part_info)
    {
        log_printf("disk read error, part_info is null device: %d", dev->minor);
        return -1;
    }
    disk_t *disk = part_info->disk;
    if (disk == (disk_t *)0)
    {
        log_printf("disk read error, disk is null device: %d", dev->minor);
        return -1;
    }
    mutex_lock(disk->mutex);
    task_on_op = 1;
    disk_send_cmd(disk, part_info->start_sector + addr, size, DISK_CMD_WRITE);

    int cnt;
    for (cnt = 0; cnt < size; cnt++, buf += disk->sector_size)
    {
        disk_write_data(disk, buf, SECTOR_SIZE);
        sem_wait(disk->op_sem);
        int err = disk_wait_data(disk);
        if (err < 0)
        {
            log_printf("disk[%s] wait data error,start sector:%d,count:%d", disk->name, addr, cnt);
            break;
        }
    }
    mutex_unlock(disk->mutex);
    return cnt;
}

/**
 * @brief 控制磁盘
 */

int disk_control(device_t *dev, int cmd, int arg0, int arg1)
{
    return -1;
}

/**
 * @brief 关闭磁盘
 */
void disk_close(device_t *dev)
{
    while (1)
        ;
}
void do_handler_ide_primary(exception_frame_t *frame)
{
    pic_send_eoi(IRQ14_HARDDISK_PRIMARY);
    if (task_on_op)
    {
        sem_signal(&op_sem);
    }
}
dev_desc_t dec_disk_desc = {
    .name = "disk",
    .major = DEV_DISK,
    .open = disk_open,
    .read = disk_read,
    .write = disk_write,
    .control = disk_control,
    .close = disk_close,
};