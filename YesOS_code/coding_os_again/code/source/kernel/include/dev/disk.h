/**
 * @FilePath     : /code/source/kernel/include/dev/disk.h
 * @Description  :  磁盘.h文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-10 16:31:18
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef DISK_H
#define DISK_H
#include "ipc/mutex.h"
#include "ipc/sem.h"
#include "comm/types.h"
#define DISK_NAME_SIZE 128                            // 磁盘名称
#define PART_NAME_SIZE 128                            // 分区名
#define DISK_PRIMARY_PART_CNT (4 + 1)                 // 分区数量
#define DISK_CNT 2                                    // 磁盘数量
#define DISK_PER_CHANNEL 2                            // 每根总线2块磁盘
#define IOBASE_PRIMARY 0x1F0                          // 主IDE通道基础端口地址
#define DISK_DATA(disk) (disk->port_base + 0)         // 数据寄存器地址
#define DISK_ERROR(disk) (disk->port_base + 1)        // 错误信息寄存器地址
#define DISK_SECTOR_COUNT(disk) (disk->port_base + 2) // 扇区计数寄存器地址
#define DISK_LBA_LOW(disk) (disk->port_base + 3)      // LBA地址低8位寄存器地址
#define DISK_LBA_MID(disk) (disk->port_base + 4)      // LBA地址中8位寄存器地址
#define DISK_LBA_HI(disk) (disk->port_base + 5)       // LBA地址高8位寄存器地址
#define DISK_DRIVE(disk) (disk->port_base + 6)        // 驱动器/磁头选择寄存器地址
#define DISK_STATUS(disk) (disk->port_base + 7)       // 状态寄存器地址
#define DISK_CMD(disk) (disk->port_base + 7)          // 命令寄存器地址

#define DISK_CMD_IDENTIFY 0xEC // 识别磁盘命令
#define DISK_CMD_READ 0x24     // 读取磁盘命令
#define DISK_CMD_WRITE 0x34    // 写磁盘命令

#define DISK_STATUS_ERR (1 << 0)  // 磁盘错误状态
#define DISK_STATUS_DRQ (1 << 3)  // 数据请求位，表示磁盘准备好进行数据传输
#define DISK_STATUS_DF (1 << 5)   // 设备故障位，表示磁盘硬件故障
#define DISK_STATUS_BUSY (1 << 7) // 磁盘忙

#define DISK_DRIVE_BASE 0xE0 // 磁盘起始地址

#define MBR_PRIMARY_PART_NR 4 // 分区表数量

/**
 * 分区信息
 */
typedef struct _part_info_t
{
    char name[PART_NAME_SIZE]; // 分区名
    struct _disk_t *disk;
    enum
    {
        FS_INVALID = 0x00, // 无效
        FS_FAT16_0 = 0x06, // fat16
        FS_FAT16_1 = 0x0E,
    } type;           // 文件系统类型
    int start_sector; // 开始扇区
    int total_sector; // 总扇区
} part_info_t;

/**
 * 磁盘结构描述符
 */
typedef struct _disk_t
{
    char name[DISK_NAME_SIZE]; // 磁盘名称
    enum
    {
        DISK_MASTER = (0 << 4),                   // 主盘
        DISK_SLAVE = (1 << 4),                    // 从盘
    } drive;                                      // 磁盘驱动类型
    uint16_t port_base;                           // i/o端口地址
    int sector_size;                              // 扇区大小
    int sector_count;                             // 扇区数量
    part_info_t part_info[DISK_PRIMARY_PART_CNT]; // 分区表
    mutex_t *mutex;                               // 互斥锁
    sem_t *op_sem;                                   // 信号量
} disk_t;

#pragma pack(1)
/**
 * 分区表表项
 */
typedef struct _part_item_t
{
    uint8_t boot_active;          // 引导标志
    uint8_t start_header;         // 分区起始位置的磁头号
    uint16_t start_sector : 6;    // 分区起始位置的磁头号
    uint16_t start_cylinder : 10; // 分区起始位置的磁头号
    uint8_t system_id;            // 分区类型标识符
    uint8_t end_header;           // 分区结束位置的磁头号
    uint16_t end_sector : 6;      // 分区结束位置的扇区号
    uint16_t end_cylinder : 10;   // 分区结束位置的柱面号
    uint32_t relative_sector;     // 分区相对于磁盘起始位置的偏移量
    uint32_t total_sectors;       // 分区的总扇区数
} part_item_t;

/**
 * mbr分区表
 */
typedef struct _mbr_t
{
    uint8_t code[446];                          // 引导代码46字节
    part_item_t part_item[MBR_PRIMARY_PART_NR]; // 分区表
    uint8_t boot_sig[2];                        // 引导标志
} mbr_t;
#pragma pack()

/**
 * @brief        : 磁盘初始化
 * @return        {void}
 **/
void disk_init(void);

void exception_handler_ide_primary(void); // 中断处理函数
#endif