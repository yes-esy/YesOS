/**
 * @FilePath     : /code/source/kernel/core/memory.c
 * @Description  : 内存管理相关实现
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-04 19:40:26
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "core/memory.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "cpu/mmu.h"
static addr_alloc_t paddr_alloc;                                               // 地址分配结构                                         // 物理地址分配结构
static pde_t kernel_page_dir[PDE_CNT] __attribute__((aligned(MEM_PAGE_SIZE))); // 内核页目录表(全局唯一)
/**
 * @brief        : 对相应的地址分配结构进行初始化
 * @param         {addr_alloc_t} *alloc: 地址分配结构的指针
 * @param         {uint8_t} *bits: 位图字节缓冲区
 * @param         {uint32_t} start: 起始地址
 * @param         {uint32_t} size: 大小
 * @param         {uint32_t} page_size: 页大小,块大小
 * @return        {*}
 **/
static void addr_alloc_init(addr_alloc_t *alloc, uint8_t *bits, uint32_t start, uint32_t size, uint32_t page_size)
{
    mutex_init(&alloc->mutex);
    alloc->start = start;
    alloc->size = size;
    alloc->page_size = page_size;
    bitmap_init(&alloc->bitmap, bits, alloc->size / page_size, 0);
}

/**
 * @brief        : 分配页存储空间
 * @param         {addr_alloc_t *} alloc: 地址分配结构
 * @param         {int} page_count: 分配的页数
 * @return        {*} :物理页地址
 **/
static uint32_t addr_alloc_page(addr_alloc_t *alloc, int page_count)
{
    uint32_t addr = 0; // 地址
    mutex_lock(&alloc->mutex);
    int page_index = bitmap_alloc_nbits(&alloc->bitmap, 0, page_count); // 找到page_count页
    if (page_index >= 0)
    {
        addr = alloc->start + page_index * alloc->page_size;
    }
    mutex_unlock(&alloc->mutex);
    return addr;
}
/**
 * @brief        : 释放已经分配的页内存
 * @param         {addr_alloc_t} *alloc: 地址分配器结构体指针
 * @param         {uint32_t} addr: 要释放的结束物理地址
 * @param         {int} page_count: 要释放的连续页数
 * @return        {void} : 无返回值
 **/
static void addr_free_page(addr_alloc_t *alloc, uint32_t addr, int page_count)
{
    mutex_lock(&alloc->mutex);                                      // 进入临界区，保证进程安全
    uint32_t page_index = (addr - alloc->start) / alloc->page_size; // 计算页在位图中的索引位置
    bitmap_set_bit(&alloc->bitmap, page_index, page_count, 0);      // 将位图对应位置设为0，标记为空闲
    mutex_unlock(&alloc->mutex);                                    // 退出临界区
}

/**
 * @brief        : 打印内存信息,内存区域序号、存储区域起始地址、存储区域大小
 * @param         {boot_info_t} *boot_info: OS启动信息
 * @return        {*}
 **/
void show_mem_inifo(boot_info_t *boot_info)
{
    log_printf("memory region:");
    for (int i = 0; i < boot_info->ram_region_count; i++)
    {
        log_printf("[%d]:0x%x-0x%x", i, boot_info->ram_region_cfg[i].start, boot_info->ram_region_cfg[i].size);
    }
    log_printf("\n");
}
/**
 * @brief        : 计算整个内存区域大小
 * @param         {boot_info_t} *boot_info: OS启动信息
 * @return        {uint32_t} :总内存大小
 **/
static uint32_t total_mem_size(boot_info_t *boot_info)
{
    uint32_t mem_size = 0;
    for (int i = 0; i < boot_info->ram_region_count; i++)
    {
        mem_size += boot_info->ram_region_cfg[i].size;
    }
    return mem_size;
}
/**
 * @brief        : 寻找对应的页表
 * @param         {pde_t} *page_dir: 页目录表地址
 * @param         {uint32_t} vaddr: 虚拟地址
 * @param         {int} alloc: 如果未找到是否分配一个表项,1 是,0 否
 * @return        {pte_t *}: 页表的地址
 **/
pte_t *find_pte(pde_t *page_dir, uint32_t vaddr, int alloc)
{
    pte_t *page_table;
    pde_t *pde = page_dir + pde_index(vaddr); // 页目录表中页目录项
    if (pde->present)                         // 判断是否存在
    {
        page_table = (pte_t *)pde_paddr(pde); // 对应的页表的物理地址
    }
    else
    {
        if (alloc == 0) // 不存在,不分配
        {
            return (pte_t *)0;
        }
        // 不存在,分配
        uint32_t pg_addr = addr_alloc_page(&paddr_alloc, 1); // 分配一个页表
        if (pg_addr == 0)                                    // 分配失败
        {
            return (pte_t *)0;
        }
        // 分配成功,对齐进行初始化
        pde->v = pg_addr | PDE_P | PDE_W | PDE_U;
        page_table = (pte_t *)pg_addr;
        kernel_memset((void *)page_table, 0, MEM_PAGE_SIZE);
    }
    return page_table + pte_index(vaddr);
}
/**
 * @brief        : 建立物理地址和虚拟地址的映射关系,指定开始的虚拟起始地址映射到物理起始地址连续count物理页
 * @param         {pde_t} *page_dir: 页目录表地址
 * @param         {uint32_t} vaddr: 线性起始地址
 * @param         {uint32_t} paddr: 物理起始地址
 * @param         {int} count: 映射多少物理页
 * @param         {uint32_t} perm: 权限字段
 * @return        {int} : 建立映射关系成功返回0,失败返回-1
 **/
int memory_create_map(pde_t *page_dir, uint32_t vaddr, uint32_t paddr, int count, uint32_t perm)
{
    for (int i = 0; i < count; i++)
    {
        // log_printf("create map:v-0x%x,p-0x%x,perm:0x%x", vaddr, paddr, perm);
        pte_t *pte = find_pte(page_dir, vaddr, 1); // 从页目录表中找到对应的页表项
        if (pte == (pte_t *)0)
        {
            // log_printf("create pte failed . pte == 0");
            // 未找到返回 -1
            return -1;
        }
        // log_printf("pte addr:0x%x", (uint32_t)pte);
        // 建立映射关系
        ASSERT(pte->present == 0);
        pte->v = paddr | perm | PTE_P;

        // 下一页的映射关系
        vaddr += MEM_PAGE_SIZE;
        paddr += MEM_PAGE_SIZE;
    }
    return 0;
}
/**
 * @brief        : 创建内核页目录表,并根据映射结构创建映射关系
 * @return        {*}
 **/
void create_kernel_table()
{
    extern uint8_t s_text[], e_text[], s_data[], kernel_base[];

    /*
     * 内核映射关系说明：
     * memory_map_t 结构: {虚拟起始地址, 虚拟结束地址, 物理起始地址, 权限标识符}
     *
     * 映射策略：
     * 1. 内核低地址区域：identity mapping (虚拟地址 = 物理地址)
     * 2. 不同区域设置不同权限以保护内核安全
     */
    static memory_map_t kernel_map[] = {
        /* 映射1: kernel_base 到 s_text 区域
         * 虚拟地址: kernel_base ~ s_text
         * 物理地址: 0x0 开始 (低地址区域)
         * 权限: PTE_W (可写)
         * 用途: 内核启动代码、中断向量表等低地址必需区域 */
        {kernel_base, s_text, 0, PTE_W},

        /* 映射2: 内核代码段
         * 虚拟地址: s_text ~ e_text
         * 物理地址: s_text (identity mapping)
         * 权限: 0 (只读，不可写)
         * 用途: 内核代码区域，只读保护防止代码被意外修改 */
        {s_text, e_text, s_text, 0},

        /* 映射3: 内核数据段
         * 虚拟地址: s_data ~ MEM_EBDA_START-1
         * 物理地址: s_data (identity mapping)
         * 权限: PTE_W (可读写)
         * 用途: 内核数据区域，包括全局变量、BSS段等 */
        {s_data, (void *)(MEM_EBDA_START - 1), s_data, PTE_W},

        /* 映射4: 扩展内存区域
         * 虚拟地址: MEM_EXT_START ~ MEM_EXT_END
         * 物理地址: MEM_EXT_START (identity mapping，一一对应)
         * 权限: PTE_W (可读写)
         * 用途: 1MB以上扩展内存，用于动态分配、用户进程等 */
        {(void *)MEM_EXT_START, (void *)MEM_EXT_END, (void *)MEM_EXT_START, PTE_W},
    };

    // 遍历表项,建立映射关系
    for (int i = 0; i < sizeof(kernel_map) / sizeof(memory_map_t); i++)
    {
        memory_map_t *map = kernel_map + i;

        // 页边界对齐
        uint32_t vstart = down2((uint32_t)map->vstart, MEM_PAGE_SIZE);
        uint32_t vend = up2((uint32_t)map->vend, MEM_PAGE_SIZE);
        uint32_t paddr = down2((uint32_t)map->pstart, MEM_PAGE_SIZE);
        // 页数量
        int page_count = (vend - vstart) / MEM_PAGE_SIZE;

        // 建立映射关系
        memory_create_map(kernel_page_dir, vstart, paddr, page_count, map->perm);
    }
}
/**
 * @brief        : 取当前进程的页目录表指针
 * @return        {pde_t *} : 当前进程的页目录表指针
 **/
static pde_t *curr_page_dir(void)
{
    return (pde_t *)(task_current()->tss.cr3);
}

/**
 * @brief        : 创建页目录表,并将内核页目录表中的表项拷贝到新创建的页目录表中
 * @return        {uint32_t}: 页目录表物理地址
 **/
uint32_t memory_create_uvm(void)
{
    pde_t *page_dir = (pde_t *)addr_alloc_page(&paddr_alloc, 1); // 分配一页内存用于存放页目录表
    if (page_dir == 0)                                           // 分配失败
    {
        return 0;
    }
    kernel_memset((void *)page_dir, 0, MEM_PAGE_SIZE);     // 清空
    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE); // 用户进程起始地址在内核页表中开始的表项索引
    for (int i = 0; i < user_pde_start; i++)               // 将内核页目录项的内容复制到新的页目录表中
    {
        page_dir[i].v = kernel_page_dir[i].v;
    }
    return (uint32_t)page_dir; // 返回页目录表物理地址
}
/**
 * @brief        : 为指定页目录的虚拟地址区域分配页表内存空间
 * @param         {uint32_t} page_dir: 页目录表的物理地址
 * @param         {uint32_t} vaddr: 需要分配内存的虚拟起始地址
 * @param         {uint32_t} size: 需要分配的内存空间大小（字节）
 * @param         {int} perm: 页面权限标志（如可读、可写、用户态等）
 * @return        {int} : 成功返回0，失败返回错误码
 **/
int memory_alloc_page_for_dir(uint32_t page_dir, uint32_t vaddr, uint32_t size, int perm)
{
    uint32_t curr_vaddr = vaddr;                               // 线性地址
    int page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE; // 需要分配的物理页数量
    vaddr = down2(vaddr, MEM_PAGE_SIZE);                       // 向下对齐到整数倍
    for (int i = 0; i < page_count; i++)                       // 逐页分配内存,然后建立映射关系
    {
        uint32_t paddr = addr_alloc_page(&paddr_alloc, 1); // 分配一页物理内存
        if (paddr == 0)                                    // 分配失败
        {
            log_printf("mem alloc failed. non memory");
            return 0;
        }
        int err = memory_create_map((pde_t *)page_dir, curr_vaddr, paddr, 1, perm); // 建立映射关系
        if (err < 0)                                                                // 建立映射关系失败
        {
            log_printf("create memory failed. err=%d", err);
            // 释放全部已建立映射关系的页
            addr_free_page(&paddr_alloc, vaddr, i + 1);
            return -1;
        }
        curr_vaddr += MEM_PAGE_SIZE; // 处理下一页
    }
    return 0;
}
/**
 * @brief        : 返回虚拟地址在页表中的物理地址
 * @param         {uint32_t} page_dir: 页目录表
 * @param         {uint32_t} vaddr: 虚拟地址
 * @return        {uint32_t} vaddr在页目录表中的物理地址
 **/
uint32_t memory_get_paddr(uint32_t page_dir, uint32_t vaddr)
{
    pte_t *pte = find_pte((pde_t *)page_dir, vaddr, 0); // 对应页表项的指针
    if (!pte)                                           // 没有找到
    {
        return 0;
    }
    return pte_paddr(pte) + (vaddr & (MEM_PAGE_SIZE - 1)); // 物理地址加偏移量
}
/**
 * @brief        : 将数据从一个地址空间拷贝到另一个进程的用户虚拟内存空间中。
 * @param         {uint32_t} to: 目标虚拟地址,在page_dir页目录表中
 * @param         {uint32_t} page_dir:  目标页目录表
 * @param         {uint32_t} from: 源地址(当前地址空间)
 * @param         {uint32_t} size: 拷贝的字节数
 * @return        {int} : 状态码. 成功返回0,失败返回-1
 **/
int memory_copy_uvm_data(uint32_t to, uint32_t page_dir, uint32_t from, uint32_t size)
{
    while (size > 0)
    {
        uint32_t to_paddr = memory_get_paddr(page_dir, to); // 目的地址所在物理地址
        if (to_paddr == 0)                                  // 目标地址未映射
        {
            return -1;
        }
        uint32_t offset_in_page = to_paddr & (MEM_PAGE_SIZE - 1); // 页内偏移
        uint32_t curr_size = MEM_PAGE_SIZE - offset_in_page;      // 当前页还能拷贝的大小
        if (curr_size > size)                                     // 如果剩余要拷贝的数据小于当前页能容纳的，则只拷贝剩余的
        {
            curr_size = size;
        }
        kernel_memcpy((void *)to_paddr, (void *)from, curr_size); // 执行实际的内存拷贝（直接使用物理地址）
        // 更新指针和剩余大小
        size -= curr_size;
        to += curr_size;
        from += curr_size;
    }
    return 0 ;
}
/**
 * @brief        : 为当前进程的虚拟地址区域分配页表内存空间
 * @param         {uint32_t} addr: 需要分配内存的虚拟起始地址
 * @param         {uint32_t} size: 需要分配的内存空间大小（字节）
 * @param         {int} perm: 页面权限标志（如可读、可写、用户态等）
 * @return        {int} : 成功返回0，失败返回错误码
 **/
int memory_alloc_page_for(uint32_t vaddr, uint32_t size, int perm)
{
    uint32_t page_dir = task_current()->tss.cr3;                   // 取当前页目录表地址
    return memory_alloc_page_for_dir(page_dir, vaddr, size, perm); // 为当前页目录表分配页表
}
/**
 * @brief        : 分配一页物理内存
 * @return        {uint32_t} : 申请到的内存空间的虚拟地址
 **/
uint32_t memory_alloc_page(void)
{
    uint32_t addr = addr_alloc_page(&paddr_alloc, 1);
    return addr;
}
/**
 * @brief        : 释放一页物理内存
 * @param         {uint32_t} addr:
 * @return        {*}
 **/
void memory_free_page(uint32_t addr)
{
    if (addr < MEMORY_TASK_BASE) // 地址小于进程起始空间,直接释放
    {
        addr_free_page(&paddr_alloc, addr, 1);
    }
    else // 超过进程起始空间,线性地址(虚拟地址)
    {
        pte_t *pte = find_pte(curr_page_dir(), addr, 0); // 对于的页表项
        ASSERT((pte != (pte_t *)0) && pte->present);
        addr_free_page(&paddr_alloc, pte_paddr(pte), 1); // 释放该表项
        pte->v = 0;                                      // 清楚映射关系,表项清空
    }
}
/**
 * @brief        : 释放页目录表及其映射关系
 * @param         {uint32_t} page_dir: 需要释放的页目录表的物理地址
 * @return        {void}
 **/
void memory_destory_uvm(uint32_t page_dir)
{
    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE); // pde表项索引
    pde_t *pde = (pde_t *)(page_dir + user_pde_start);     // 得到原一级页目录表项
    for (int i = user_pde_start; i < PDE_CNT; i++, pde++)  // 从用户进程空间开始
    {
        if (!pde->present) // 页目录表项不存在
        {
            continue;
        }
        pte_t *pte = (pte_t *)pde_paddr(pde);    // 取出页表物理地址
        for (int j = 0; j < PTE_CNT; j++, pte++) // 遍历页表
        {
            if (!pte->present) // 页表项不存在
            {
                continue;
            }
            addr_free_page(&paddr_alloc, pte_paddr(pte), 1); // 释放该页物理地址
        }
        addr_free_page(&paddr_alloc, (uint32_t)pde_paddr(pde), 1); // 释放该页表对应的空间
    }
    addr_free_page(&paddr_alloc, (uint32_t)page_dir, 1); // 释放该页目录表
}
/**
 * @brief        : 复制一个页目录表及其映射关系，创建一个新的页目录表
 * @param         {uint32_t} page_dir: 需要复制的源页目录表的物理地址
 * @return        {uint32_t} : 新创建的页目录表的物理地址，失败返回-1
 **/
uint32_t memory_copy_uvm(uint32_t page_dir)
{
    uint32_t to_page_dir = memory_create_uvm(); // 创建页目录表
    if (to_page_dir == 0)                       // 创建失败
    {
        goto copy_uvm_failed;
    }
    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE); // pde表项索引
    pde_t *pde = (pde_t *)page_dir + user_pde_start;       // 得到原一级页目录表中用户进程开始的页目录表项
    for (int i = user_pde_start; i < PDE_CNT; i++, pde++)  // 遍历页目录表
    {
        if (!pde->present) // 页目录表项不存在
        {
            continue;
        }
        pte_t *pte = (pte_t *)pde_paddr(pde);    // 取出页目录表项对应的页表的物理地址
        for (int j = 0; j < PTE_CNT; j++, pte++) // 遍历页表
        {
            if (!pte->present) // 页表项不存在
            {
                continue;
            }
            uint32_t page = addr_alloc_page(&paddr_alloc, 1); // 分配一页物理内存
            if (page == 0)                                    // 分配失败
            {
                goto copy_uvm_failed;
            }
            uint32_t vaddr = (i << 22) | (j << 12);
            int err = memory_create_map((pde_t *)to_page_dir, vaddr, page, 1, get_pte_perm(pte)); // 建立映射关系
            if (err < 0)
            {
                goto copy_uvm_failed;
            }
            kernel_memcpy((void *)page, (void *)vaddr, MEM_PAGE_SIZE); // 复制内容
        }
    }
    return to_page_dir;
copy_uvm_failed:
    if (to_page_dir)
    {
        memory_destory_uvm(to_page_dir); // 释放页目录表
    }
    return -1;
}
/**
 * @brief        : 初始化
 * @param         {boot_info_t} *boot_info:启动信息
 * @return        {*}
 **/
void memory_init(boot_info_t *boot_info)
{
    extern uint8_t *mem_free_start; // 指定外部符号,放在bss后面
    log_printf("memory init...");

    show_mem_inifo(boot_info); // 打印启动信息

    uint8_t *mem_free = (uint8_t *)&mem_free_start; // 指向OS后面的内存地址

    uint32_t mem_up1MB_free = total_mem_size(boot_info) - MEM_EXT_START; // 1MB以上内存区域大小

    mem_up1MB_free = down2(mem_up1MB_free, MEM_PAGE_SIZE); // 对齐到4KB, 4KB页大小的整数倍

    log_printf("free memory:0x%x,size:0x%x", MEM_EXT_START, mem_up1MB_free); // 打印内存信息

    addr_alloc_init(&paddr_alloc, mem_free, MEM_EXT_START, mem_up1MB_free, MEM_PAGE_SIZE); // 初始化地址分配结构,再kernel之后

    mem_free += bitmap_byte_count(paddr_alloc.size / MEM_PAGE_SIZE); // 跳过位图缓存

    ASSERT(mem_free < (uint8_t *)MEM_EBDA_START);

    create_kernel_table(); // 创建内核页表

    mmu_set_page_dir((uint32_t)kernel_page_dir); // 设置页目录表地址,打开分页机制
    log_printf("memory success.");
}