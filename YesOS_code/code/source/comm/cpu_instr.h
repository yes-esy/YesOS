/**
 * @file cpu_instr.h
 * @brief CPU指令相关函数
 * @version 0.1
 * @date 2025-02-02
 * @author yes
 *
 */
#ifndef CPU_INSTR_H
#define CPU_INSTR_H

#include "types.h"

/**
 * @brief 从端口读取一个字节
 *
 * @param port 端口号
 * @return uint8_t 读取的字节
 */
static inline uint8_t inb(uint16_t port)
{
    uint8_t rv;
    __asm__ __volatile__("inb %[p], %[v]" : [v] "=a"(rv) : [p] "d"(port));
    return rv;
}
/**
 * @brief 从端口写入一个字节
 *
 */
static inline uint16_t inw(uint16_t port)
{
    uint16_t rv;
    __asm__ __volatile__("in %1, %0" : "=a"(rv) : "dN"(port));
    return rv;
}

/**
 * @brief 向端口写入一个字节
 *
 * @param port 端口号
 * @param data 写入的数据
 */
static inline void outb(uint16_t port, uint8_t data)
{
    __asm__ __volatile__("outb %[v], %[p]" : : [p] "d"(port), [v] "a"(data));
}

/**
 * @brief 关中断
 */
static inline void cli()
{
    __asm__ __volatile__("cli");
}

/**
 * @brief 开中断
 */
static inline void sti()
{
    __asm__ __volatile__("sti");
}

/**
 * @brief 加载gdt
 */
static inline void lgdt(uint32_t start, uint32_t size)
{
    struct
    {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_16;
    } gdt;

    gdt.start31_16 = start >> 16;
    gdt.start15_0 = start & 0xFFFF;
    gdt.limit = size - 1;

    __asm__ __volatile__("lgdt %[g]" ::[g] "m"(gdt));
}

/**
 * @brief 加载idt
 */
static inline void lidt(uint32_t start, uint32_t size)
{
    struct
    {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_16;
    } idt;

    idt.start31_16 = start >> 16;
    idt.start15_0 = start & 0xFFFF;
    idt.limit = size - 1;

    __asm__ __volatile__("lidt %[g]" ::[g] "m"(idt));
}

/**
 * @brief 读cr0
 *
 * @return uint16_t
 */
static inline uint32_t read_cr0()
{
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %[v]" : [v] "=r"(cr0));
    return cr0;
}
/**
 * @brief 写cr0
 *
 * @param v
 */
static inline void write_cr0(uint32_t v)
{
    __asm__ __volatile__("mov %[v], %%cr0" ::[v] "r"(v));
}

static inline void far_jump(uint32_t selector, uint32_t offset)
{
    uint32_t addr[] = {offset, selector};
    __asm__ __volatile__("ljmpl *(%[a])" ::[a] "r"(addr));
}

static inline void hlt()
{
    __asm__ __volatile__("hlt");
}

static inline void write_tr(uint32_t tss_selector)
{
    __asm__ __volatile__("ltr %%ax" ::"a"(tss_selector));
}

static inline uint32_t read_eflags(void)
{
    uint32_t eflags;

    __asm__ __volatile__("pushfl\n\tpopl %%eax" : "=a"(eflags));
    return eflags;
}

static inline void write_eflags(uint32_t eflags)
{
    __asm__ __volatile__("pushl %%eax\n\tpopfl" ::"a"(eflags));
}

/**
 * @brief 读cr3
 *
 * @return uint16_t
 */
static inline uint32_t read_cr3()
{
    uint32_t cr3;
    __asm__ __volatile__("mov %%cr3, %[v]" : [v] "=r"(cr3));
    return cr3;
}
/**
 * @brief 写cr3
 *
 * @param v
 */
static inline void write_cr3(uint32_t v)
{
    __asm__ __volatile__("mov %[v], %%cr3" ::[v] "r"(v));
}

/**
 * @brief 读cr3
 *
 * @return uint16_t
 */
static inline uint32_t read_cr4()
{
    uint32_t cr4;
    __asm__ __volatile__("mov %%cr4, %[v]" : [v] "=r"(cr4));
    return cr4;
}
/**
 * @brief 写cr4
 *
 * @param v
 */
static inline void write_cr4(uint32_t v)
{
    __asm__ __volatile__("mov %[v], %%cr4" ::[v] "r"(v));
}

/**
 * @brief 读cr2
 *
 * @return uint32_t
 */
static inline uint32_t read_cr2()
{
    uint32_t cr2;
    __asm__ __volatile__("mov %%cr2, %[v]" : [v] "=r"(cr2));
    return cr2;
}
/**
 * @brief 写cr2
 *
 * @param v
 */
static inline void write_cr2(uint32_t v)
{
    __asm__ __volatile__("mov %[v], %%cr2" ::[v] "r"(v));
}

static inline void outw(uint16_t port, uint16_t data)
{
    __asm__ __volatile__("out %[v], %[p]" : : [p] "d"(port), [v] "a"(data));
}
#endif