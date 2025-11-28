; -------------------------------------------------------------------
; Created by ShipOS developers
; Copyright (c) 2023 SHIPOS. All rights reserved.
;
; This file defines the assembly stubs for CPU interrupts and exceptions.
; It provides a common entry point for all interrupts and handles
; saving/restoring CPU registers before calling the C handler.
; -------------------------------------------------------------------

extern interrupt_handler   ; declare the common C interrupt handler

; -------------------------------------------------------------------
; Macros for defining interrupt handlers
; -------------------------------------------------------------------

; -------------------------------------------------------------------
; no_error_code_interrupt_handler <INT_NUM>
; -------------------------------------------------------------------
; Generates a handler for interrupts/exceptions that do NOT push an error code
; on the stack. It pushes 0 as the error code and the interrupt number
; to be passed to the C handler.

%macro no_error_code_interrupt_handler 1
global interrupt_handler_%1
interrupt_handler_%1:
    mov   rdi, qword 0          ; push 0 as error code
    mov   rsi, qword %1         ; push the interrupt number
    jmp     common_interrupt_handler
%endmacro

; -------------------------------------------------------------------
; error_code_interrupt_handler <INT_NUM>
; -------------------------------------------------------------------
; Generates a handler for interrupts/exceptions that automatically
; push an error code on the stack. Pops the error code into rdi
; and pushes the interrupt number into rsi for the C handler.

%macro error_code_interrupt_handler 1
global interrupt_handler_%1
interrupt_handler_%1:
    pop rdi                     ; retrieve error code
    mov rsi, qword %1           ; push the interrupt number
    jmp     common_interrupt_handler
%endmacro

; -------------------------------------------------------------------
; common_interrupt_handler
; -------------------------------------------------------------------
; Saves all general-purpose registers, calls the common C handler,
; then restores registers and returns from the interrupt.

common_interrupt_handler:
    ; Save general-purpose registers
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Call C interrupt handler
    call interrupt_handler

    ; Restore general-purpose registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Return to interrupted code
    iret

; -------------------------------------------------------------------
; Instantiate all interrupt handlers
; -------------------------------------------------------------------

no_error_code_interrupt_handler 0
no_error_code_interrupt_handler 1
no_error_code_interrupt_handler 2
no_error_code_interrupt_handler 3
no_error_code_interrupt_handler 4
no_error_code_interrupt_handler 5
no_error_code_interrupt_handler 6
no_error_code_interrupt_handler 7
error_code_interrupt_handler 8
no_error_code_interrupt_handler 9
error_code_interrupt_handler 10
error_code_interrupt_handler 11
error_code_interrupt_handler 12
error_code_interrupt_handler 13
error_code_interrupt_handler 14
no_error_code_interrupt_handler 15
no_error_code_interrupt_handler 16
error_code_interrupt_handler 17
no_error_code_interrupt_handler 18
no_error_code_interrupt_handler 19
no_error_code_interrupt_handler 20
error_code_interrupt_handler 21
no_error_code_interrupt_handler 22
no_error_code_interrupt_handler 23
no_error_code_interrupt_handler 24
no_error_code_interrupt_handler 25
no_error_code_interrupt_handler 26
no_error_code_interrupt_handler 27
no_error_code_interrupt_handler 28
error_code_interrupt_handler 29
error_code_interrupt_handler 30
no_error_code_interrupt_handler 31
