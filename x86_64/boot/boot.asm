global start
extern kernel_main
extern check_multiboot
extern check_cpuid
extern check_long_mode
section .text
bits 32
start:
    mov esp, stack_top
    call check_multiboot
    call check_cpuid
    call check_long_mode
    jmp page_tables_setup

page_tables_setup:
    ; point first entry of p4 table to the first entry in p3 table
    mov eax, p3_table
    ; make first two bits 1
    ; present bit - page is currently in memory
    ; writable bit - page allowed to be written to
    or eax, 0b11
    mov dword [p4_table + 0], eax
    mov eax, p2_table
    or eax, 0b11
    mov dword [p3_table + 0], eax

    mov eax, p1_table
    or eax, 0b11
    mov dword [p2_table + 0], eax

    mov ecx, 0 ; counter
.map_p1_table:
    mov eax, 0x1000  ; Physical address for this page
    mul ecx
    or eax, 0b11       ; Set Present (P), Read/Write (R/W), and User (U/S) flags
    mov [p1_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512        ; 512 entries in a page table
    jl .map_p1_table



.setup_page_register:
    ; move page table address to cr3
    ; using eax because we can move data to control register
    ; only from another register
    mov eax, p4_table
    mov cr3, eax

.enable_pae:
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    ; set the long mode bit
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

.enable_paging:
    mov eax, cr0
    or eax, 1 << 31
    or eax, 1 << 16
    mov cr0, eax



.update_lgdr_register:
    lgdt [gdt64.pointer]

.update_selectors:
    mov ax, gdt64.data
    mov ss, ax
    mov ds, ax
    mov es, ax

.jmp_long_mode:
    jmp gdt64.code:long_mode_start
    hlt


section .bss
align 4096

p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096
p1_table:
    resb 4096
stack_bottom:
    resb 4096*8
stack_top:

section .rodata
gdt64:
    dq 0

.code: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41) | (1<<43) | (1<<53)

.data: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41)

.pointer:
    dw .pointer - gdt64 - 1
    dq gdt64

    ; LONG MODE
section .text
bits 64
long_mode_start:
    mov rsp, stack_top

    call kernel_main

    hlt