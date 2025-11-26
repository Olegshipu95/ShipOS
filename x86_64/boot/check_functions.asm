global check_multiboot
global check_cpuid
global check_long_mode
bits 32
check_multiboot:
    cmp eax, 0x36d76289 ; magic value (if multiboot comparable)
    jne .no_multiboot
    ret
.no_multiboot:
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