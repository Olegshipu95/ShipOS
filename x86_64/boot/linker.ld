ENTRY(start)

SECTIONS{
    /* AP trampoline */
    . = 0x8000;
    .ap_trampoline :
    {
        *(.ap_trampoline)
    }

    /* Kernel */
    . = 1M;

    .boot :
    {
        *(.header_start)
    }

    .text :
    {
        *(.text)
    }


    .bss : 
    {
        *(.bss)
    }

    PROVIDE(end = .);
}