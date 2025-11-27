//
// Created by ShipOS developers on 22.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//
// Serial port (COM1) driver for kernel debugging and logging.
// Provides text output to serial port which can be redirected by QEMU.
//

#ifndef SHIPOS_SERIAL_H
#define SHIPOS_SERIAL_H

#include <inttypes.h>

/**
 * @brief Utility macro for logging into serial port
 * 
 * @param logger Name to be displayed in square brackets
 * @param format Format string
 * @param ... Variadic arguments
 */
#define LOG_SERIAL(logger, format, ...) serial_printf("[" logger "] " format "\n\r", ##__VA_ARGS__)

// Several presets for serial logging
#define DEBUG_SERIAL(format, ...) LOG_SERIAL("DEBUG", format, ##__VA_ARGS__)
#define PANIC_SERIAL(format, ...) LOG_SERIAL("PANIC", format, ##__VA_ARGS__)
#define LOG_BOOT_SERIAL(format, ...) LOG_SERIAL("BOOT", format, ##__VA_ARGS__)
#define LOG_TTY_SERIAL(format, ...) LOG_SERIAL("TTY", format, ##__VA_ARGS__)
#define LOG_MEM_SERIAL(format, ...) LOG_SERIAL("MEMORY", format, ##__VA_ARGS__)
#define LOG_PROC_SERIAL(format, ...) LOG_SERIAL("PROCESS", format, ##__VA_ARGS__)
#define LOG_INT_SERIAL(format, ...) LOG_SERIAL("INTERRUPT", format, ##__VA_ARGS__)
#define LOG_KER_SERIAL(format, ...) LOG_SERIAL("KERNEL", format, ##__VA_ARGS__)

/**
 * @brief COM1 base port address
 */
#define SERIAL_COM1_PORT 0x3F8

/**
 * @brief Initialize serial port COM1 with standard settings:
 * - Baud rate: 38400
 * - Data bits: 8
 * - Stop bits: 1
 * - Parity: None
 * - FIFO enabled
 * 
 * @return 0 on success, -1 on failure (faulty serial port)
 */
int init_serial();

/**
 * @brief Check if transmit buffer is empty and ready for writing
 * @return 1 if ready, 0 otherwise
 */
int serial_is_transmit_empty();

/**
 * @brief Write a single character to serial port
 * Blocks until transmit buffer is ready
 * @param c Character to write
 */
void serial_putchar(char c);

/**
 * @brief Write a null-terminated string to serial port
 * @param str String to write
 */
void serial_write(const char *str);

/**
 * @brief Formatted print to serial port (supports %d, %x, %o, %b, %p, %s, %%)
 * Thread-safe implementation with spinlock
 * @param format Format string
 * @param ... Variadic arguments
 */
void serial_printf(const char* format, ...);

#endif //SHIPOS_SERIAL_H
