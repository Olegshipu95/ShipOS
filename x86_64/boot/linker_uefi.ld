/* Linker script for UEFI boot */
/* Entry point is the 64-bit UEFI entry, not the 32-bit multiboot entry */

ENTRY(uefi_kernel_entry)

SECTIONS {
    /* Load at 1MB - standard location for kernels */
    . = 0x100000;

    .text : ALIGN(4096) {
        /* UEFI entry comes first */
        *uefi_entry*.o(.text)
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
