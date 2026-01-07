ENTRY(kernel_main)

SECTIONS {
    /* Load at 1MB - standard location for kernels */
    . = 0x100000;

    .text : ALIGN(4096) {
        *(.text)
        *(.text.*)
    }

    .rodata : ALIGN(4096) {
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(4096) {
        *(.data)
        *(.data.*)
    }

    .bss : ALIGN(4096) {
        PROVIDE(__bss_start = .);
        *(COMMON)
        *(.bss)
        *(.bss.*)
        PROVIDE(__bss_end = .);
    }

    PROVIDE(end = .);
}
