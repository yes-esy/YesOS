/**
 * @FilePath     : /code/source/kernel/include/fs/fatfs/fatfs.h
 * @Description  :  fat文件系统头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-12 09:53:41
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef FAT_FS_H
#define FAT_FS_H
#include "comm/types.h"
#define DIRITEM_NAME_FREE 0xE5      // 目录项空闲名标记
#define DIRITEM_NAME_END 0x00       // 目录项结束名标记
#define DIRITEM_ATTR_READ_ONLY 0x01 // 目录项属性：只读
#define DIRITEM_ATTR_HIDDEN 0x02    // 目录项属性：隐藏
#define DIRITEM_ATTR_SYSTEM 0x04    // 目录项属性：系统类型
#define DIRITEM_ATTR_VOLUME_ID 0x08 // 目录项属性：卷id
#define DIRITEM_ATTR_DIRECTORY 0x10 // 目录项属性：目录
#define DIRITEM_ATTR_ARCHIVE 0x20   // 目录项属性：归档
#define DIRITEM_ATTR_LONG_NAME 0x0F // 目录项属性：长文件名
#define SFN_LEN 11                  // sfn文件名长
#define FAT_CLUSTER_INVALID 0xFFF8  // 簇编号无效
#define FAT_CLUSTER_FREE 0x00       // 空闲或无效的簇号

#pragma pack(1)

/**
 * FAT目录结构
 */
typedef struct _diritem_t
{
    uint8_t DIR_Name[11];      // 文件名
    uint8_t DIR_Attr;          // 属性
    uint8_t DIR_NTRes;         // NT 保留字段
    uint8_t DIR_CrtTimeTeenth; // 创建时间的毫秒
    uint16_t DIR_CrtTime;      // 创建时间
    uint16_t DIR_CrtDate;      // 创建日期
    uint16_t DIR_LastAccDate;  // 最后访问日期
    uint16_t DIR_FstClusHI;    // 簇号高16位
    uint16_t DIR_WrtTime;      // 修改时间
    uint16_t DIR_WrtDate;      // 修改时期
    uint16_t DIR_FstClusL0;    // 簇号低16位
    uint32_t DIR_FileSize;     // 文件字节大小
} diritem_t;

/**
 * FAT16文件系统配置信息结构
 */
typedef struct _dbr_t
{
    uint8_t BS_jmpBoot[3];   // 跳转代码
    uint8_t BS_OEMName[8];   // OEM名称
    uint16_t BPB_BytsPerSec; // 每扇区字节数
    uint8_t BPB_SecPerClus;  // 每簇扇区数
    uint16_t BPB_RsvdSecCnt; // 保留区扇区数
    uint8_t BPB_NumFATs;     // FAT表项数
    uint16_t BPB_RootEntCnt; // 根目录项目数
    uint16_t BPB_TotSec16;   // 总的扇区数
    uint8_t BPB_Media;       // 媒体类型
    uint16_t BPB_FATSz16;    // FAT表项大小
    uint16_t BPB_SecPerTrk;  // 每磁道扇区数
    uint16_t BPB_NumHeads;   // 磁头数
    uint32_t BPB_HiddSec;    // 隐藏扇区数
    uint32_t BPB_TotSec32;   // 总的扇区数

    uint8_t BS_DrvNum;         // 磁盘驱动器参数
    uint8_t BS_Reserved1;      // 保留字节
    uint8_t BS_BootSig;        // 扩展引导标记
    uint32_t BS_VolID;         // 卷标序号
    uint8_t BS_VolLab[11];     // 磁盘卷标
    uint8_t BS_FileSysType[8]; // 文件类型名称
} dbr_t;
#pragma pack()
/**
 * FAT表描述符
 */
typedef struct _fat_t
{
    uint32_t table_start;       // fat表起始扇区号
    uint32_t table_cnt;         // fat表数量
    uint32_t table_sectors;     // fat表扇区数量
    uint32_t bytes_per_sec;     // 每扇区的字节数
    uint32_t sec_per_cluster;   // 每扇区的簇数量
    uint32_t root_ent_cnt;      // 根目录区域可以容纳的最大文件或目录条目数量
    uint32_t root_start;        // 根目录区域的起始扇区号。
    uint32_t data_start;        // 数据区的起始扇区号
    struct _fs_t *fs;           // 文件系统结构体 (fs_t) 的指
    uint8_t *fat_buf;           // fat缓冲区
    uint32_t cluster_byte_size; // 一个簇的字节数
    int curr_sector;            // 当前fat_buf缓冲区保存的是哪一个扇区的数据
} fat_t;
typedef uint16_t cluster_t; // 簇类型

#endif