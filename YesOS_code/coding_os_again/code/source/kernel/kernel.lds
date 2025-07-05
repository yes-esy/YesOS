SECTIONS {
    /**
    * 内核代码从物理地址 0x10000 开始
    * EXCLUDE_FILE(*first_task*) 排除了所有包含 "first_task" 的文件的代码段
    * s_text 标记代码段开始地址
    */
    PROVIDE(kernel_base = .);
    . = 0x00010000;
    PROVIDE(s_text = .);
    .text : {
        	*(EXCLUDE_FILE(*first_task* *lib_syscall*) .text)
    }
    /**
    * 存放常量数据
    * e_text 标记只读区域结束
    **/
    .rodata :{
        *(EXCLUDE_FILE(*first_task* *lib_syscall*) .rodata)
    }
    PROVIDE(e_text = .);
    /**
    * 按 4KB 对齐（页面边界）
    * .data 存放初始化的全局变量
    * .bss 存放未初始化的全局变量
    * e_data 标记数据区结束
    **/
    . = ALIGN(4096);
    PROVIDE(s_data = .);
    .data :{
        *(EXCLUDE_FILE(*first_task* *lib_syscall*) .data)
    }
    .bss : {
        *(EXCLUDE_FILE(*first_task* *lib_syscall*) .bss)
    }
    e_data = .;
    /**
    * 虚拟地址: 0x80000000 (用户空间起始地址)
    * 物理地址: AT(e_data) 指定加载到内核数据段之后
    * 内容: 包含所有 first_task 相关文件的所有段
    **/
    . = 0x80000000;
    PROVIDE(s_first_task = LOADADDR(.first_task));
    .first_task :AT(e_data) {
        *first_task_entry*(.text .rodata .bss .data)
        *first_task*(.text .rodata .bss .data)
        *lib_syscall*(.text .data .bss)

    }
    PROVIDE(e_first_task = LOADADDR(.first_task) + SIZEOF(.first_task));
    PROVIDE(mem_free_start = e_first_task );

    /**
    * e_first_task 计算第一个任务的物理结束地址
    * mem_free_start 提供给内核的空闲内存起始地址
    **/
}