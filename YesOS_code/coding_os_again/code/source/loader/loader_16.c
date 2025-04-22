/**
 * @FilePath     : /code/source/loader/loader_16.c
 * @Description  :  16位模式下的loader, * 16位引导代码
 * 二级引导，负责进行硬件检测，进入保护模式，然后加载内核，并跳转至内核运行
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-13 21:13:22
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
__asm__(".code16gcc"); // 按16位模式编译文件

#include "loader.h"
#include "comm/cpu_instr.h"

boot_info_t boot_info; // 启动参数信息

/**
 * @brief        : 打印信息
 * @param         {char} *msg:需要打印的字符串
 * @return        {*}
 **/
static void show_msg(const char *msg)
{
    char c;
    while ((c = *msg++) != '\0')
    {
        /**
         * mov $0xe,%ah\n\t
         * mov $'L',%al\n\t
         * mov int $0x10
         */
        __asm__ __volatile__("mov $0xe,%%ah\n\t"
                             "mov %[ch],%%al\n\t"
                             "int $0x10" ::[ch] "r"(c));
    }
}

/**
 * @brief        : 检测内存容量
 * @return        {*}
 **/
static void detect_memory(void)
{
    uint32_t contID = 0;
    SMAP_entry_t smap_entry;
    int signature, bytes;

    show_msg("try to detect memory:");

    // 初次：EDX=0x534D4150,EAX=0xE820,ECX=24,INT 0x15, EBX=0（初次）
    // 后续：EAX=0xE820,ECX=24,
    // 结束判断：EBX=0
    boot_info.ram_region_count = 0;
    for (int i = 0; i < BOOT_RAM_REGION_MAX; i++)
    {
        SMAP_entry_t *entry = &smap_entry;

        __asm__ __volatile__("int  $0x15"
                             : "=a"(signature), "=c"(bytes), "=b"(contID)
                             : "a"(0xE820), "b"(contID), "c"(24), "d"(0x534D4150), "D"(entry));
        if (signature != 0x534D4150)
        {
            show_msg("detect failedly.\r\n");
            return;
        }

        // todo: 20字节
        if (bytes > 20 && (entry->ACPI & 0x0001) == 0)
        {
            continue;
        }

        // 保存RAM信息，只取32位，空间有限无需考虑更大容量的情况
        if (entry->Type == 1)
        {
            boot_info.ram_region_cfg[boot_info.ram_region_count].start = entry->BaseL;
            boot_info.ram_region_cfg[boot_info.ram_region_count].size = entry->LengthL;
            boot_info.ram_region_count++;
        }

        if (contID == 0)
        {
            break;
        }
    }
    show_msg("detect successfully.\r\n");
}

// gdt表,全局描述符
uint16_t gdt_table[][4] = {
    {0, 0, 0, 0},
    {0xFFFF, 0x0000, 0x9A00, 0x00CF},
    {0xFFFF, 0x0000, 0x9200, 0x00CF},
};

static void enter_protect_mode()
{
    // 关中断
    cli();

    // 开启A20地址线,使得可以访问1M以上的空间
    uint8_t v = inb(0x92);
    outb(0x92, v | 0x2);

    // 设置gdt表
    lgdt((uint32_t)gdt_table, sizeof(gdt_table));

    // 读取cr0
    uint32_t cr0 = read_cr0();
    // 写cr0
    write_cr0(cr0 | (1 << 0));

    // 远跳转指令
    far_jump(8, (uint32_t)protect_mode_entry);
}

/**
 * @brief        : 16位下的loader
 * @return        {*}
 **/
void loader_entry(void)
{
    show_msg("loader is running....\r\n");
    detect_memory();
    enter_protect_mode();
    for (;;)
    {
    }
}
