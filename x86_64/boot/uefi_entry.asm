; UEFI entry point for kernel
; This is called directly from UEFI bootloader in 64-bit Long Mode
; UEFI has already set up paging, but we need our own page tables
; and GDT before calling kernel_main

global uefi_kernel_entry
extern kernel_main
extern __bss_start
extern end

section .text
bits 64

uefi_kernel_entry:
    ; Disable interrupts during setup
    cli
    
    ; First, clear BSS section (objcopy doesn't include it in binary)
    lea rdi, [rel __bss_start]
    lea rcx, [rel end]
    sub rcx, rdi
    shr rcx, 3              ; Divide by 8 (clear qwords)
    xor rax, rax
    rep stosq
    
    ; Set up our own stack (now BSS is cleared, stack area is safe)
    lea rsp, [rel uefi_stack_top]
    
    ; Set up page tables for identity mapping
    call setup_page_tables
    
    ; Load our GDT
    lea rax, [rel gdt64.pointer]
    lgdt [rax]
    
    ; Reload segment registers
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far jump to reload CS with our code segment
    push gdt64.code
    lea rax, [rel .reload_cs]
    push rax
    retfq
    
.reload_cs:
    ; Call kernel main
    call kernel_main
    
    ; Should not return, but if it does - halt
.halt:
    cli
    hlt
    jmp .halt

; Set up identity mapping for first 4GB
; Uses 2MB pages for simplicity
setup_page_tables:
    ; Clear page tables (BSS already cleared, but let's be safe)
    lea rdi, [rel pml4_table]
    xor rax, rax
    mov rcx, 4096 * 5 / 8  ; 5 tables (pml4 + pdpt + 4*pd), 4096 bytes each
    rep stosq
    
    ; PML4[0] -> PDPT
    lea rax, [rel pdpt_table]
    or rax, 0b11            ; Present + Writable
    lea rdi, [rel pml4_table]
    mov [rdi], rax
    
    ; PDPT[0..3] -> PD tables (4GB mapping)
    lea rdi, [rel pdpt_table]
    lea rax, [rel pd_table]
    or rax, 0b11
    mov [rdi], rax
    
    lea rax, [rel pd_table + 4096]
    or rax, 0b11
    mov [rdi + 8], rax
    
    lea rax, [rel pd_table + 8192]
    or rax, 0b11
    mov [rdi + 16], rax
    
    lea rax, [rel pd_table + 12288]
    or rax, 0b11
    mov [rdi + 24], rax
    
    ; Fill PD tables with 2MB pages (identity mapping)
    lea rdi, [rel pd_table]
    mov rax, 0b10000011     ; Present + Writable + Page Size (2MB)
    mov rcx, 512 * 4        ; 512 entries * 4 tables = 2048 entries = 4GB
    
.fill_pd:
    mov [rdi], rax
    add rax, 0x200000       ; Next 2MB
    add rdi, 8
    dec rcx
    jnz .fill_pd
    
    ; Load new page table
    lea rax, [rel pml4_table]
    mov cr3, rax
    
    ret

section .bss
align 4096

pml4_table:
    resb 4096
pdpt_table:
    resb 4096
pd_table:
    resb 4096 * 4           ; 4 page directory tables for 4GB mapping

uefi_stack_bottom:
    resb 4096 * 16          ; 64KB stack
uefi_stack_top:

section .rodata
align 16
gdt64:
    dq 0                    ; Null descriptor

.code: equ $ - gdt64
    ; Code segment: Execute/Read, 64-bit, Present, DPL=0
    dq (1<<44) | (1<<47) | (1<<41) | (1<<43) | (1<<53)

.data: equ $ - gdt64
    ; Data segment: Read/Write, Present, DPL=0
    dq (1<<44) | (1<<47) | (1<<41)

.pointer:
    dw $ - gdt64 - 1        ; GDT size - 1
    dq gdt64                ; GDT address
