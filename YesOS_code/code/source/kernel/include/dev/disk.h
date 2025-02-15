#ifndef DISK_H
#define DISK_H

#include "comm/types.h"
#include "ipc/mutex.h"
#include "ipc/sem.h"
#define PART_NAME_SIZE 32 // 分区名称
#define DISK_NAME_SIZE 32
#define DISK_PRIMARY_PART_CNT (4 + 1)
#define DISK_CNT 2
#define DISK_PER_CHANNEL 2

#define IOBASE_PRIMARY 0x1F0
#define DISK_DATA(disk) (disk->port_base + 0)
#define DISK_ERROR(disk) (disk->port_base + 1)
#define DISK_SECTOR_COUNT(disk) (disk->port_base + 2)
#define DISK_LBA_LOW(disk) (disk->port_base + 3)
#define DISK_LBA_MID(disk) (disk->port_base + 4)
#define DISK_LBA_HIGH(disk) (disk->port_base + 5)
#define DISK_DRIVE(disk) (disk->port_base + 6)
#define DISK_CMD(disk) (disk->port_base + 7) // 命令寄存器
#define DISK_STATUS(disk) (disk->port_base + 7)

#define DISK_CMD_READ 0x24
#define DISK_CMD_WRITE 0x34
#define DISK_CMD_IDENTIFY 0xEC

#define DISK_STATUS_ERR (1 << 0)
#define DISK_STATUS_DRQ (1 << 3)
#define DISK_STATUS_DF (1 << 5)
#define DISK_STATUS_BUSY (1 << 7)

#define DISK_DRIVE_BASE 0xE0

#define MBR_PRIMARY_PART_NR 4 // 4个分区表

#pragma pack(1)
/**
 * MBR的分区表项类型
 */
typedef struct _part_item_t
{
    uint8_t boot_active;          // 分区是否活动
    uint8_t start_header;         // 起始header
    uint16_t start_sector : 6;    // 起始扇区
    uint16_t start_cylinder : 10; // 起始磁道
    uint8_t system_id;            // 文件系统类型
    uint8_t end_header;           // 结束header
    uint16_t end_sector : 6;      // 结束扇区
    uint16_t end_cylinder : 10;   // 结束磁道
    uint32_t relative_sectors;    // 相对于该驱动器开始的相对扇区数
    uint32_t total_sectors;       // 总的扇区数
} part_item_t;

typedef struct _mbr_t
{
    uint8_t code[446];
    part_item_t part_item[MBR_PRIMARY_PART_NR];
    uint8_t boot_sig[2];

} mbr_t;

struct _disk_t;

typedef struct _partinfo_t
{
    char name[PART_NAME_SIZE];
    struct _disk_t *disk;
    enum
    {
        FS_INVALID = 0x00,
        FS_FAT16_0 = 0x6,
        FS_FAT16_1 = 0xE,
    } type;

    int start_sector;
    int total_sector;

} partinfo_t;

typedef struct _disk_t
{
    char name[DISK_NAME_SIZE];

    enum
    {
        DISK_MASTER = (0 << 4),
        DISK_SLAVE = (1 << 4),
    } drive;

    uint16_t port_base;
    int sector_size;
    int sector_count;
    partinfo_t partinfo[DISK_PRIMARY_PART_CNT];

    mutex_t *mutex;
    sem_t *op_sem;
} disk_t;
#pragma pack()
void disk_init(void);

void exception_handler_ide_primary(void);
#endif