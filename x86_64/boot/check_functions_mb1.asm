global check_multiboot1
global check_cpuid
global check_long_mode
bits 32
check_multiboot1:
    cmp eax, 0x2BADB002 ; Multiboot1 bootloader magic value
    jne .no_multiboot1
    ret
.no_multiboot1:
    mov al, "M"
    jmp error
check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov al, "C"
    jmp error
check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    mov al, "L"
    jmp error

error:
    ; print "ERR: X" where x - error code
    mov word [0xb8000], 0x4f524f45
    mov word [0xb8002], 0x4f3a4f52
    mov word [0xb8004], 0x4f204f20
    mov byte [0xb800a], al
