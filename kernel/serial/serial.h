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
#define LOG_SERIAL(logger, format, ...) serial_printf(get_default_serial_port(), "[" logger "] " format "\n\r", ##__VA_ARGS__)

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
 * @brief Standard base port addresses for COM ports
 */
#define SERIAL_COM1_PORT 0x3F8
#define SERIAL_COM2_PORT 0x2F8
#define SERIAL_COM3_PORT 0x3E8
#define SERIAL_COM4_PORT 0x2E8

/**
 * @brief Maximum number of supported serial ports
 */
#define MAX_SERIAL_PORTS 4

/**
 * @brief Detect and enumerate available serial ports.
 * Probes standard COM port addresses and tests them using loopback mode.
 * 
 * @param ports Array to store detected port addresses (must be at least MAX_SERIAL_PORTS size)
 * @return Number of detected ports (0 to MAX_SERIAL_PORTS)
 */
int detect_serial_ports(uint16_t *ports);

/**
 * @brief Initialize a specific serial port with standard settings:
 * - Baud rate: 38400
 * - Data bits: 8
 * - Stop bits: 1
 * - Parity: None
 * - FIFO enabled
 * 
 * @param port Base I/O port address
 * @return 0 on success, -1 on failure (faulty serial port)
 */
int init_serial(uint16_t port);

/**
 * @brief Set the default serial port for logging macros
 * 
 * @param port Base I/O port address of the default port
 */
void set_default_serial_port(uint16_t port);

/**
 * @brief Get the default serial port for logging macros
 * 
 * @return Base I/O port address of the default port
 */
uint16_t get_default_serial_port();

/**
 * @brief Check if transmit buffer is empty and ready for writing on a specific port
 * 
 * @param port Base I/O port address
 * @return 1 if ready, 0 otherwise
 */
int serial_is_transmit_empty(uint16_t port);

/**
 * @brief Write a single character to a specific serial port
 * Blocks until transmit buffer is ready
 * 
 * @param port Base I/O port address
 * @param c Character to write
 */
void serial_putchar(uint16_t port, char c);

/**
 * @brief Write a null-terminated string to a specific serial port
 * 
 * @param port Base I/O port address
 * @param str String to write
 */
void serial_write(uint16_t port, const char *str);

/**
 * @brief Formatted print to a specific serial port (supports %d, %x, %o, %b, %p, %s, %%)
 * Thread-safe implementation with spinlock
 * 
 * @param port Base I/O port address
 * @param format Format string
 * @param ... Variadic arguments
 */
void serial_printf(uint16_t port, const char* format, ...);

#endif //SHIPOS_SERIAL_H