/**
 * @FilePath     : /code/source/loader/loader_32.c
 * @Description  :  32位保护模式下的loader
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-06-26 14:54:02
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "loader.h"
#include "comm/cpu_instr.h"
#include "comm/elf.h"

/**
 * @brief        : 使用LBA48模式读取磁盘
 * @param         {uint32_t} sector: 扇区号
 * @param         {uint32_t} sector_count: 扇区数量
 * @param         {uint8_t *} buff: 输入缓冲区
 * @return        {*} 程序入口地址
 **/
static void read_disk(uint32_t sector, uint32_t sector_count, uint8_t *buf)
{
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
}

/**
 * @brief        : 从指定地址解析elf文件
 * @param         {uint8_t *} file_buffer: elf文件的位置
 * @return        {*}
 **/
static uint32_t reload_elf_file(uint8_t *file_buffer)
{
    // 读取的只是ELF文件，不像BIN那样可直接运行，需要从中加载出有效数据和代码
    // 简单判断是否是合法的ELF文件
    Elf32_Ehdr *elf_hdr = (Elf32_Ehdr *)file_buffer;
    if ((elf_hdr->e_ident[0] != ELF_MAGIC) || (elf_hdr->e_ident[1] != 'E') || (elf_hdr->e_ident[2] != 'L') || (elf_hdr->e_ident[3] != 'F'))
    {
        return 0;
    }

    // 然后从中加载程序头，将内容拷贝到相应的位置
    for (int i = 0; i < elf_hdr->e_phnum; i++)
    {
        Elf32_Phdr *phdr = (Elf32_Phdr *)(file_buffer + elf_hdr->e_phoff) + i;
        if (phdr->p_type != PT_LOAD)
        {
            continue;
        }

        // 全部使用物理地址，此时分页机制还未打开
        uint8_t *src = file_buffer + phdr->p_offset;
        uint8_t *dest = (uint8_t *)phdr->p_paddr;
        for (int j = 0; j < phdr->p_filesz; j++)
        {
            *dest++ = *src++;
        }

        // memsz和filesz不同时，后续要填0
        dest = (uint8_t *)phdr->p_paddr + phdr->p_filesz;
        for (int j = 0; j < phdr->p_memsz - phdr->p_filesz; j++)
        {
            *dest++ = 0;
        }
    }

    return elf_hdr->e_entry;
}

/**
 * @brief        : 死机指令
 * @param         {int} code: 错误码
 * @return        {*}
 **/
static void die(int code)
{
    for (;;)
    {
    }
}

static void enable_page_mode()
{
#define PDE_P (1 << 0) // 表项有效
#define PDE_W (1 << 1) // 可写
#define PDE_PS (1 << 7)
#define CR4_PSE (1 << 4)
#define CR0_PG (1 << 31)
    static uint32_t page_dir[1024] __attribute__((aligned(4096))) = {
        [0] = PDE_P | PDE_PS | PDE_W|0, // PDE_PS，开启4MB的页，对表项0进行处理，因为loader对应表项0所在的0~4MB的区域。
    }; // 页目录表

    // 设置PSE，以便启用4M的页，而不是4KB
    uint32_t cr4 = read_cr4(); // 读出CR4
    write_cr4(cr4 | CR4_PSE);  // 写CR4

    // 设置页表地址
    write_cr3((uint32_t)page_dir); // 写CR3

    // 开启分页机制
    write_cr0(read_cr0() | CR0_PG); // 写CR0
}

void load_kernel(void)
{

    read_disk(100, 500, (uint8_t *)SYS_KERNEL_LOAD_ADDR);

    uint32_t kernel_entry = reload_elf_file((uint8_t *)SYS_KERNEL_LOAD_ADDR);
    // 加载失败
    if (kernel_entry == 0)
    {
        die(-1);
    }

    enable_page_mode();
    ((void (*)(boot_info_t *))kernel_entry)(&boot_info);
    for (;;)
    {
    }
}