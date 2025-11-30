ENTRY(start)

SECTIONS{

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