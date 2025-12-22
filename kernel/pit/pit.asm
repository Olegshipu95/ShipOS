; -----------------------------------------------------------------------------
; Created by untitled-os developers
; Copyright (c) 2023 untitled-os. All rights reserved.
;
; This file implements the PIT (Programmable Interval Timer) setup and control.
; It provides functions to initialize the PIT, send values to the scheduler,
; and stop the timer.
; -----------------------------------------------------------------------------

global init_pit
global send_values_to_sched
global stop_timer

section .text
align 4

; -----------------------------------------------------------------------------
; @brief Initialize the PIT (channel 0) with mode 3 (square wave generator)
; The PIT is used for generating timer interrupts at a programmable frequency.
; -----------------------------------------------------------------------------
init_pit:
    ; bits:
    ; 0     - Binary mode (0 = Binary, 1 = BCD)
    ; 1:3   - Operating mode (3 = Square wave generator)
    ; 4:5   - Read/Write mode (11 = lobyte/hibyte)
    ; 6:7   - Channel selection (00 = channel 0)
    mov al, 00111000b
    ; Port 43 for setting the timer
    out 0x43, al    ;tell the PIT which channel we're setting

; -----------------------------------------------------------------------------
; @brief Send a 16-bit reload value to PIT channel 0
;
; The PIT (Programmable Interval Timer) expects a 16-bit value that determines
; the timer interval. This value is split into two bytes:
;   - Low byte (AL) is sent first
;   - High byte (AH) is sent second
;
; Formula for timer frequency:
;   frequency (Hz) = 1193182 / reload_value
;
; In this example:
;   reload_value = 0b0111111111111111 = 0x7FFF = 32767 (decimal)
;   frequency ≈ 1193182 / 32767 ≈ 36.43 Hz
;
; This means the timer will generate approximately 36 interrupts per second.
; -----------------------------------------------------------------------------
send_values_to_sched:
    mov ax, 0b0111111111111111
    out 0x40, al    ;send low byte
    mov al, ah
    out 0x40, al    ;send high byte
    ret

; -----------------------------------------------------------------------------
; @brief Stop the PIT timer (channel 0)
; Sets the timer value to 0 to stop generating interrupts.
; -----------------------------------------------------------------------------
stop_timer:
    mov ax, 0
    out 0x40, al    ;send low byte
    mov al, ah
    out 0x40, al    ;send high byte
    ret
