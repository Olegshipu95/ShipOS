; AP (Application Processor) Trampoline Code
; This code runs in real mode at address 0x8000 (page 0x08)

[BITS 16]
section .ap_trampoline

global ap_trampoline_start
global ap_trampoline_end

ap_trampoline_start:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    ; Load our own GDT with 32-bit code segment
    ; lgdt [0x80A0] - manually encoded for 16-bit mode
    db 0x0F, 0x01, 0x16          ; lgdt [disp16]
    dw 0x80A0                    ; address of ap_gdt_ptr
    
    ; Enable protected mode
    mov eax, cr0
    or al, 1
    mov cr0, eax
    
    ; Far jump to 32-bit code using selector 0x08 (32-bit code segment)
    db 0xEA
    dw 0x8030                    ; offset
    dw 0x0008                    ; segment selector

; Padding to align 32-bit section at 0x30
times 0x30-($-$$) db 0x90

[BITS 32]
ap_trampoline_32:
    ; Load 32-bit data segments (selector 0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Load CR3 - manually encode mov eax, [0x80E0]
    db 0xA1
    dd 0x80E0
    mov cr3, eax
    
    ; Enable PAE
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax
    
    ; Enable long mode (EFER.LME)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr
    
    ; Enable paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    
    ; Far jump to 64-bit code using selector 0x18 (64-bit code segment)
    db 0xEA
    dd 0x8070                    ; offset (32-bit)
    dw 0x0018                    ; segment selector (64-bit code)

; Padding to align 64-bit section at 0x70
times 0x70-($-$$) db 0x90

[BITS 64]
ap_trampoline_64:
    ; Load 64-bit data segments (selector 0x10 works for data in long mode)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Load stack pointer from 0x80E8
    mov rsp, [0x80E8]
    
    ; Load entry point from 0x80F0
    mov rax, [0x80F0]
    
    ; Call the entry point
    call rax
    
    ; Should never return
    cli
    hlt

; Padding to align data section at 0xA0
times 0xA0-($-$$) db 0x90

; ============================================================================
; Data section - starts at offset 0xA0 (address 0x80A0)
; ============================================================================

; GDT pointer at 0x80A0
ap_gdt_ptr:
    dw ap_gdt_end - ap_gdt - 1  ; limit
    dd 0x80B0                    ; base address (will point to ap_gdt)
    dd 0                         ; padding for 64-bit

; Align GDT to 0xB0
times 0xB0-($-$$) db 0

; GDT at 0x80B0 - must have 32-bit and 64-bit code segments
ap_gdt:
    ; Null descriptor (selector 0x00)
    dq 0
    
    ; 32-bit Code segment (selector 0x08)
    dw 0xFFFF       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base middle
    db 0x9A         ; Access: P=1, DPL=0, S=1, Type=1010 (exec/read)
    db 0xCF         ; Flags: G=1, D=1, Limit high=0xF
    db 0x00         ; Base high
    
    ; Data segment (selector 0x10)
    dw 0xFFFF       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base middle
    db 0x92         ; Access: P=1, DPL=0, S=1, Type=0010 (read/write)
    db 0xCF         ; Flags: G=1, B=1, Limit high=0xF
    db 0x00         ; Base high
    
    ; 64-bit Code segment (selector 0x18)
    dw 0x0000       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base middle
    db 0x9A         ; Access: P=1, DPL=0, S=1, Type=1010 (exec/read)
    db 0x20         ; Flags: G=0, L=1, D=0
    db 0x00         ; Base high
ap_gdt_end:

; Align to 0xE0
times 0xE0-($-$$) db 0

; CR3 at 0x80E0
ap_cr3:
    dq 0

; Stack at 0x80E8
ap_stack:
    dq 0

; Entry at 0x80F0
ap_entry_addr:
    dq 0

ap_trampoline_end:
