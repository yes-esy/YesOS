/**
 * @FilePath     : /code/source/loader/loader.h
 * @Description  :  loader.h头文件及配置项
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-07 17:38:04
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef LOADER_H
#define LOADER_H

#include "comm/boot_info.h"
#include "comm/types.h"


// 申明保护模式入口函数,在start.S中定义
void protect_mode_entry(void);

// 内存检测信息结构
typedef struct SMAP_entry
{
    uint32_t BaseL; // base address uint64_t
    uint32_t BaseH;
    uint32_t LengthL; // length uint64_t
    uint32_t LengthH;
    uint32_t Type; // entry Type
    uint32_t ACPI; // extended
} __attribute__((packed)) SMAP_entry_t;

extern boot_info_t boot_info;
#endif
