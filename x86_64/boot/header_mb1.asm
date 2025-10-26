header_start:
    dd 0x1BADB002                ; Multiboot1 magic number
    dd 0x00000003                ; flags: page align + mem info
    dd 0xE4524FFB                ; checksum: -(magic + flags)

header_end: