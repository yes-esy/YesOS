SECTIONS {
    . = 0x00010000;

    .text : {
        *(.text)
    }

    .data :{
        *(.data)
    }

    .rodata :{
        *(.rodata)
    }

    .bss : {
        *(.bss)
    }
}