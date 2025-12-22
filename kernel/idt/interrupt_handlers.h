//
// Created by untitled-os developers
// Copyright (c) 2023 untitled-os. All rights reserved.
//
// This header declares all interrupt handlers for untitled-os kernel.
// It includes keyboard, timer, default, and CPU exception handlers.
//

#ifndef UNTITLED_OS_INTERRUPT_HANDLERS_H
#define UNTITLED_OS_INTERRUPT_HANDLERS_H
//#include "../lib/include/stdint.h"
#include <inttypes.h>
#include "../lib/include/x86_64.h"

/**
 * @brief Keyboard interrupt wrapper
 *
 * Assembly wrapper for the keyboard handler, sets up stack and
 * calls keyboard_handler().
 */
void keyboard_handler_wrapper();

/**
 * @brief Keyboard interrupt handler
 *
 * Handles key presses from the keyboard. Switches virtual terminals
 * when function keys F1-F7 are pressed. Other key codes are printed.
 */
void keyboard_handler();

/**
 * @brief Timer interrupt handler
 *
 * Sends timing information to the scheduler, performs context switching
 * between threads, and sends End-of-Interrupt (EOI) to the PIC.
 */
void timer_interrupt();

/**
 * @brief Default interrupt handler
 *
 * Handles unknown or unhandled interrupts.
 * Prints a generic message when an unknown interrupt occurs.
 */
void default_handler();

/**
 * @brief General CPU exception handler
 *
 * @param error_code Error code from CPU (if any)
 * @brief General interrupt handler for CPU exceptions
 *
 * Prints information about the interrupt number, human-readable
 * description, and error code. Also prints the CR2 register for
 * page fault exceptions. Halts the system after printing.
 *
 */
void interrupt_handler(uint64_t, uint64_t);

void interrupt_handler_0();
void interrupt_handler_1();
void interrupt_handler_2();
void interrupt_handler_3();
void interrupt_handler_4();
void interrupt_handler_5();
void interrupt_handler_6();
void interrupt_handler_7();
void interrupt_handler_8();
void interrupt_handler_9();
void interrupt_handler_10();
void interrupt_handler_11();
void interrupt_handler_12();
void interrupt_handler_13();
void interrupt_handler_14();
void interrupt_handler_15();
void interrupt_handler_16();
void interrupt_handler_17();
void interrupt_handler_18();
void interrupt_handler_19();
void interrupt_handler_20();
void interrupt_handler_21();
void interrupt_handler_22();
void interrupt_handler_23();
void interrupt_handler_24();
void interrupt_handler_25();
void interrupt_handler_26();
void interrupt_handler_27();
void interrupt_handler_28();
void interrupt_handler_29();
void interrupt_handler_30();
void interrupt_handler_31();

#endif //UNTITLED_OS_INTERRUPT_HANDLERS_H
