__asm__(".code16gcc"); // 16位实模式

#include "loader.h"
boot_info_t boot_info;

/**
 * @brief 显示消息
 */
static void show_msg(const char *msg)
{
    char c;
    while ((c = *msg++) != '\0')
    {
        __asm__ __volatile__(
            "mov $0xe, %%ah\n\t"
            "mov %[ch], %%al\n\t"
            "int $0x10" ::[ch] "r"(c));
    }
}
/**
 * @brief 检测内存
 */
static void detect_memory(void)
{
    SMAP_entry_t smap_entry;
    uint32_t signature, bytes, countID = 0;
    show_msg("Detecting memory(please wait):\n\r");

    boot_info.ram_region_count = 0;
    for (int i = 0; i < BOOT_RAM_REGION_MAX; i++)
    {
        SMAP_entry_t *entry = &smap_entry;
        __asm__ __volatile__("int  $0x15"
                             : "=a"(signature), "=c"(bytes), "=b"(countID)
                             : "a"(0xE820), "b"(countID), "c"(24), "d"(0x534D4150), "D"(entry));
        if (signature != 0x534D4150) // SMAP
        {
            show_msg("Detect memory failed\r\n");
            return;
        }
        if (bytes > 20 && (entry->ACPI & 0x0001) == 0) // bit0=0时表明此条目应当被忽略
        {
            continue;
        }
        if (entry->Type == 1) // entry Type，值为1时表明为我们可用的RAM空间
        {
            boot_info.ram_region_cfg[boot_info.ram_region_count].start = entry->BaseL;
            boot_info.ram_region_cfg[boot_info.ram_region_count].size = entry->LengthL;
            boot_info.ram_region_count++;
        }

        if (countID == 0) // 读取完毕
        {
            break;
        }
    }
    show_msg("Detect memory success\n\r");
}
uint16_t gdt_table[][4] = {
    {0, 0, 0, 0},
    {0xFFFF, 0x0000, 0x9A00, 0x00CF},
    {0xFFFF, 0x0000, 0x9200, 0x00CF},
};
/**
 * @brief 进入保护模式
 */
static void enter_protect_mode()
{
    cli();

    uint8_t v = inb(0x92);
    outb(0x92, v | 0x2); // 关闭A20

    lgdt((uint32_t)gdt_table, sizeof(gdt_table));


    uint32_t cr0 = read_cr0();
    write_cr0(cr0 | 1 <<0); // 设置PE位
    far_jump(8,(uint32_t)protect_mode_entry);
}

void loader_entry(void)
{
    show_msg("...loading...\n\r");
    detect_memory();
    enter_protect_mode();
    for (;;)
    {
    }
}