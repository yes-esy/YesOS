ENTRY(_start)
SECTIONS
{
    . = 0x82000000;
    .text : {
        *(*.text)
    }
    .rodata :{
        *(*.rodata)
    }
    .data : {
        *(*.data)
    }
    .bss : {
        /*bss起始地址*/
        __bss__start__ = .;
        *(*.bss)
        /*bss结束地址*/
        __bss__end__ = .;
        
    }
}