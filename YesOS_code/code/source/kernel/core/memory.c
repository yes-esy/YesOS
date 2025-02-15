#include "core/memory.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "cpu/mmu.h"
#include "dev/console.h"
#include "fs/file.h"
static addr_alloc_t paddr_alloc;

static pde_t kernel_page_dir[PDE_CNT] __attribute__((aligned(MEM_PAGE_SIZE)));

/**
 * @brief 初始化地址分配结构
 * 以下不检查start和size的页边界，由上层调用者检查
 */
static void addr_alloc_init(addr_alloc_t *addr_alloc, uint8_t *bits, uint32_t start, uint32_t size, uint32_t page_size)
{
    mutex_init(&addr_alloc->mutex);
    addr_alloc->start = start;
    addr_alloc->size = size;
    addr_alloc->page_size = page_size;
    bitmap_init(&addr_alloc->bitmap, bits, size / page_size, 0);
}

/**
 * @brief 分配多页内存
 */
static uint32_t addr_alloc_page(addr_alloc_t *addr_alloc, int page_count)
{
    uint32_t addr = 0;
    mutex_lock(&addr_alloc->mutex);

    int page_index = bitmap_alloc_nbits(&addr_alloc->bitmap, 0, page_count);

    if (page_index >= 0)
    {
        addr = addr_alloc->start + page_index * addr_alloc->page_size;
    }

    mutex_unlock(&addr_alloc->mutex);
    return addr;
}

/**
 * @brief 释放多页内存
 */
static void addr_free_page(addr_alloc_t *addr_alloc, uint32_t addr, int page_count)
{
    mutex_lock(&addr_alloc->mutex);

    int page_index = (addr - addr_alloc->start) / addr_alloc->page_size;
    bitmap_set_bit(&addr_alloc->bitmap, page_index, page_count, 0);

    mutex_unlock(&addr_alloc->mutex);
}

static void show_mem_info(boot_info_t *boot_info)
{
    // log_printf("mem region:");
    for (int i = 0; i < boot_info->ram_region_count; i++)
    {
        log_printf("[%d]: 0x%x - 0x%x", i,
                   boot_info->ram_region_cfg[i].start,
                   boot_info->ram_region_cfg[i].size);
    }
}

static int total_mem_size(boot_info_t *boot_info)
{
    uint32_t total_size = 0;
    for (int i = 0; i < boot_info->ram_region_count; i++)
    {
        total_size += boot_info->ram_region_cfg[i].size;
    }
    return total_size;
}

pte_t *find_pte(pde_t *page_dir, uint32_t vaddr, int alloc)
{
    pte_t *page_table;
    pde_t *pde = page_dir + pde_index(vaddr);

    if (pde->present)
    {
        page_table = (pte_t *)pde_paddr(pde);
    }
    else
    {
        if (alloc == 0)
        {
            return (pte_t *)0;
        }
        uint32_t pg_paddr = addr_alloc_page(&paddr_alloc, 1);

        if (pg_paddr == 0)
        {
            return (pte_t *)0;
        }

        // 设置为用户可读写，将被pte中设置所覆盖
        pde->v = pg_paddr | PDE_P | PDE_W | PDE_U;

        // 为物理页表绑定虚拟地址的映射，这样下面就可以计算出虚拟地址了
        // kernel_pg_last[pde_index(vaddr)].v = pg_paddr | PTE_P | PTE_W;

        // 清空页表，防止出现异常
        // 这里虚拟地址和物理地址一一映射，所以直接写入
        page_table = (pte_t *)(pg_paddr);
        kernel_memset(page_table, 0, MEM_PAGE_SIZE);
    }

    return page_table + pte_index(vaddr);
}

/**
 * @brief 将指定的地址空间进行一页的映射
 */
int memory_create_map(pde_t *page_dir, uint32_t vaddr, uint32_t paddr, int count, uint32_t perm)
{
    for (int i = 0; i < count; i++)
    {
        pte_t *pte = find_pte(page_dir, vaddr, 1);

        if (pte == (pte_t *)0)
        {
            return -1;
        }

        // 创建映射的时候，这条pte应当是不存在的。
        // 如果存在，说明可能有问题
        // log_printf("\tpte addr: 0x%x", (uint32_t)pte);
        ASSERT(pte->present == 0);

        pte->v = paddr | perm | PTE_P;

        vaddr += MEM_PAGE_SIZE;
        paddr += MEM_PAGE_SIZE;
    }
}
void create_kernel_table(void)
{
    extern uint8_t s_text[], e_text[], s_data[], kernel_base[];
    static memory_map_t kernel_map[] = {
        {kernel_base, s_text, 0, PTE_W},
        {s_text, e_text, s_text, 0},
        {s_data, (void *)MEM_EBDA_START, s_data, PTE_W},
        {(void *) CONSOLE_DISP_ADDR, (void *)CONSOLE_DISP_END, (void *)CONSOLE_DISP_ADDR, PTE_W},
        {(void *)MEM_EXT_START, (void *)MEM_EXT_END, (void *)MEM_EXT_START, PTE_W},
    };
    // 清空页目录表
    kernel_memset(kernel_page_dir, 0, sizeof(kernel_page_dir));

    for (int i = 0; i < sizeof(kernel_map) / sizeof(memory_map_t); i++)
    {
        // 可能有多个页，建立多个页的配置
        // 简化起见，不考虑4M的情况
        memory_map_t *map = kernel_map + i;

        uint32_t v_start = down2((uint32_t)map->vstart, MEM_PAGE_SIZE);

        uint32_t vend = up2((uint32_t)map->vend, MEM_PAGE_SIZE);

        uint32_t paddr = down2((uint32_t)map->pstart, MEM_PAGE_SIZE);

        int page_count = (vend - v_start) / MEM_PAGE_SIZE;

        memory_create_map(kernel_page_dir, v_start, (uint32_t)map->pstart, page_count, map->perm);
    }
}

/**
 * @brief 初始化内存管理系统
 * 该函数的主要任务：
 * 1、初始化物理内存分配器：将所有物理内存管理起来. 在1MB内存中分配物理位图
 * 2、重新创建内核页表：原loader中创建的页表已经不再合适
 */
void memory_init(boot_info_t *boot_info)
{
    // 1MB内存空间起始，在链接脚本中定义
    extern uint8_t *mem_free_start;

    // log_printf("mem init.");
    // show_mem_info(boot_info);

    // 在内核数据后面放物理页位图
    uint8_t *mem_free = (uint8_t *)&mem_free_start; // 2022年-10-1 经同学反馈，发现这里有点bug，改了下

    // 计算1MB以上空间的空闲内存容量，并对齐的页边界
    uint32_t mem_up1MB_free = total_mem_size(boot_info) - MEM_EXT_START;
    mem_up1MB_free = down2(mem_up1MB_free, MEM_PAGE_SIZE); // 对齐到4KB页
    // log_printf("Free memory: 0x%x, size: 0x%x", MEM_EXT_START, mem_up1MB_free);

    // 4GB大小需要总共4*1024*1024*1024/4096/8=128KB的位图, 使用低1MB的RAM空间中足够
    // 该部分的内存仅跟在mem_free_start开始放置
    addr_alloc_init(&paddr_alloc, mem_free, MEM_EXT_START, mem_up1MB_free, MEM_PAGE_SIZE);
    mem_free += bitmap_byte_count(paddr_alloc.size / MEM_PAGE_SIZE);

    // 到这里，mem_free应该比EBDA地址要小
    ASSERT(mem_free < (uint8_t *)MEM_EBDA_START);

    create_kernel_table();

    mmu_set_page_dir((uint32_t)kernel_page_dir);
}

uint32_t memory_create_uvm()
{
    pde_t *page_dir = (pde_t *)addr_alloc_page(&paddr_alloc, 1);

    if (page_dir == 0)
    {
        return 0;
    }

    kernel_memset((void *)page_dir, 0, MEM_PAGE_SIZE);

    // 复制整个内核空间的页目录项，以便与其它进程共享内核空间
    // 用户空间的内存映射暂不处理，等加载程序时创建
    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE);

    for (int i = 0; i < user_pde_start; i++)
    {
        page_dir[i].v = kernel_page_dir[i].v;
    }

    return (uint32_t)page_dir;
}

uint32_t memory_alloc_for_page_dir(uint32_t page_dir, uint32_t vaddr, uint32_t size, int perm)
{
    uint32_t curr_vaddr = vaddr;
    int page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    vaddr = down2(vaddr, MEM_PAGE_SIZE);

    // 逐页分配内存，然后建立映射关系
    for (int i = 0; i < page_count; i++)
    {
        // 分配需要的内存
        uint32_t paddr = addr_alloc_page(&paddr_alloc, 1);
        if (paddr == 0)
        {
            log_printf("mem alloc failed. no memory");
            return 0;
        }

        // 建立分配的内存与指定地址的关联
        int err = memory_create_map((pde_t *)page_dir, curr_vaddr, paddr, 1, perm);
        if (err < 0)
        {
            log_printf("create memory map failed. err = %d", err);
            addr_free_page(&paddr_alloc, vaddr, i + 1);
            return -1;
        }

        curr_vaddr += MEM_PAGE_SIZE;
    }

    return 0;
}

/**
 * @brief 为指定的虚拟地址空间分配多页内存
 */
int memory_alloc_page_for(uint32_t addr, uint32_t size, int perm)
{
    return memory_alloc_for_page_dir(task_current()->tss.cr3, addr, size, perm);
}

uint32_t memory_alloc_page(void)
{
    uint32_t addr = addr_alloc_page(&paddr_alloc, 1);
    if (addr == 0)
    {
        log_printf("memory alloc failed. no memory");
    }
    return addr;
}
static pde_t *curr_page_dir(void)
{
    return (pde_t *)(task_current()->tss.cr3);
}
void memory_free_page(uint32_t addr)
{
    if (addr < MEMORY_TASK_BASE)
    {
        addr_free_page(&paddr_alloc, addr, 1);
    }
    else
    {
        pte_t *pte = find_pte(curr_page_dir(), addr, 0);
        ASSERT(pte != (pte_t *)0 && pte->present);

        addr_free_page(&paddr_alloc, pte->phy_page_addr, 1);
        pte->v = 0;
    }
}

/**
 * @brief 销毁用户空间内存
 */
void memory_destroy_uvm(uint32_t page_dir)
{
    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE);
    pde_t *pde = (pde_t *)page_dir + user_pde_start;

    ASSERT(page_dir != 0);

    // 释放页表中对应的各项，不包含映射的内核页面
    for (int i = user_pde_start; i < PDE_CNT; i++, pde++)
    {
        if (!pde->present)
        {
            continue;
        }

        // 释放页表对应的物理页 + 页表
        pte_t *pte = (pte_t *)pde_paddr(pde);
        for (int j = 0; j < PTE_CNT; j++, pte++)
        {
            if (!pte->present)
            {
                continue;
            }

            addr_free_page(&paddr_alloc, pte_paddr(pte), 1);
        }

        addr_free_page(&paddr_alloc, (uint32_t)pde_paddr(pde), 1);
    }

    // 页目录表
    addr_free_page(&paddr_alloc, page_dir, 1);
}

/**
 * @brief 复制页表及其所有的内存空间
 */
uint32_t memory_copy_uvm(uint32_t page_dir)
{
    // 复制基础页表
    uint32_t to_page_dir = memory_create_uvm();
    if (to_page_dir == 0)
    {
        goto copy_uvm_failed;
    }

    // 再复制用户空间的各项
    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE);
    pde_t *pde = (pde_t *)page_dir + user_pde_start;

    // 遍历用户空间页目录项
    for (int i = user_pde_start; i < PDE_CNT; i++, pde++)
    {
        if (!pde->present)
        {
            continue;
        }

        // 遍历页表
        pte_t *pte = (pte_t *)pde_paddr(pde);
        for (int j = 0; j < PTE_CNT; j++, pte++)
        {
            if (!pte->present)
            {
                continue;
            }

            // 分配物理内存
            uint32_t page = addr_alloc_page(&paddr_alloc, 1);
            if (page == 0)
            {
                goto copy_uvm_failed;
            }

            // 建立映射关系
            uint32_t vaddr = (i << 22) | (j << 12);
            int err = memory_create_map((pde_t *)to_page_dir, vaddr, page, 1, get_pte_perm(pte));
            if (err < 0)
            {
                goto copy_uvm_failed;
            }

            // 复制内容。
            kernel_memcpy((void *)page, (void *)vaddr, MEM_PAGE_SIZE);
        }
    }
    return to_page_dir;

copy_uvm_failed:
    if (to_page_dir)
    {
        memory_destroy_uvm(to_page_dir);
    }
    return -1;
}

uint32_t memory_get_paddr(uint32_t page_dir, uint32_t vaddr)
{
    pte_t *pte = find_pte((pde_t *)page_dir, vaddr, 0);
    if (!pte)
    {
        return 0;
    }
    return pte_paddr(pte) + (vaddr & (MEM_PAGE_SIZE - 1));
}

int memor_copy_uvm_data(uint32_t to, uint32_t page_dir, uint32_t from, uint32_t size)
{
    while (size > 0)
    {
        uint32_t to_paddr = memory_get_paddr(page_dir, to);
        if (!to_paddr)
        {
            return -1;
        }

        uint32_t offset_in_page = to_paddr & (MEM_PAGE_SIZE - 1);

        uint32_t curr_size = MEM_PAGE_SIZE - offset_in_page;

        if (curr_size > size)
        {
            curr_size = size;
        }

        kernel_memcpy((void *)to_paddr, (void *)from, curr_size);

        size -= curr_size;
        to += curr_size;
        from += curr_size;
    }
    return 0;
}

char *sys_sbrk(int incr)
{
    int pre_incr = incr;
    task_t *task = task_current();
    char * pre_heap_end = (char *)task->heap_end;
    ASSERT(incr >= 0);
    if(incr == 0)
    {
        log_printf("sbrk(0),end=0x%x\n",pre_heap_end);
        return pre_heap_end;
    }
    uint32_t start = task->heap_end;
    uint32_t end = start + incr;

    int start_offset = start % MEM_PAGE_SIZE;
    if(start_offset)
    {
        if(start_offset + incr <= MEM_PAGE_SIZE)
        {
            task->heap_end = end;
            return pre_heap_end;
        }
        else
        {
            uint32_t curr_size = MEM_PAGE_SIZE - start_offset;
            start += curr_size;
            incr -= curr_size;
        }
    }
    if(incr)
    {
        uint32_t curr_size = end - start;
        int err = memory_alloc_page_for(start, curr_size, PTE_P | PTE_W | PTE_U);
        if(err < 0)
        {
            log_printf("sbrk failed\n");
            return (char *)-1;
        }
    }
    // log_printf("sbrk(%d),end=0x%x\n",pre_incr,task->heap_end);
    task->heap_end = end;
    return (char *)pre_heap_end;
}

