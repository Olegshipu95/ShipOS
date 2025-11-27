//
// Created by ShipOS developers on 22.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//
// Serial port (COM1) driver implementation for kernel debugging and logging.
//

#include "serial.h"
#include <stdarg.h>
#include "../lib/include/x86_64.h"
#include "../lib/include/str_utils.h"
#include "../sync/spinlock.h"

/**
 * @brief Serial port register offsets from base port
 */
#define SERIAL_DATA_PORT(base)          (base)
#define SERIAL_FIFO_COMMAND_PORT(base)  (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base)  (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base)   (base + 5)

/**
 * @brief Constants for serial port configuration
 */
#define SERIAL_DISABLE_INTERRUPTS       0x00
#define SERIAL_ENABLE_DLAB              0x80
#define SERIAL_BAUD_DIVISOR_LOW         0x03  // Low byte for divisor 3 (38400 baud)
#define SERIAL_BAUD_DIVISOR_HIGH        0x00  // High byte for divisor 3
#define SERIAL_LINE_CONFIG_8N1          0x03  // 8 bits, no parity, 1 stop bit
#define SERIAL_FIFO_ENABLE_CLEAR_14B    0xC7  // Enable FIFO, clear buffers, 14-byte threshold
#define SERIAL_MODEM_IRQ_RTS_DSR        0x0B  // IRQs enabled, RTS/DSR set
#define SERIAL_MODEM_LOOPBACK           0x1E  // Loopback mode for testing
#define SERIAL_TEST_PAYLOAD             0xAE  // Test byte for serial chip verification
#define SERIAL_MODEM_NORMAL_OP          0x0F  // Normal operation mode with IRQs
#define SERIAL_TRANSMIT_EMPTY_MASK      0x20  // Mask for transmit holding register empty bit

/**
 * @brief Spinlock for serial_printf to ensure thread-safe output
 */
static struct spinlock serial_printf_spinlock;

/**
 * @brief Flag indicating if serial port is initialized
 */
static int serial_initialized = 0;

int init_serial(int32_t port) {
    // Disable all interrupts
    outb(SERIAL_COM1_PORT + 1, SERIAL_DISABLE_INTERRUPTS);

    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_LINE_COMMAND_PORT(SERIAL_COM1_PORT), SERIAL_ENABLE_DLAB);

    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_DATA_PORT(SERIAL_COM1_PORT), SERIAL_BAUD_DIVISOR_LOW);

    // Set divisor to 3 (hi byte)
    outb(SERIAL_COM1_PORT + 1, SERIAL_BAUD_DIVISOR_HIGH);

    // 8 bits, no parity, one stop bit
    outb(SERIAL_LINE_COMMAND_PORT(SERIAL_COM1_PORT), SERIAL_LINE_CONFIG_8N1);

    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_FIFO_COMMAND_PORT(SERIAL_COM1_PORT), SERIAL_FIFO_ENABLE_CLEAR_14B);

    // IRQs enabled, RTS/DSR set
    outb(SERIAL_MODEM_COMMAND_PORT(SERIAL_COM1_PORT), SERIAL_MODEM_IRQ_RTS_DSR);

    // Set in loopback mode, test the serial chip
    outb(SERIAL_MODEM_COMMAND_PORT(SERIAL_COM1_PORT), SERIAL_MODEM_LOOPBACK);

    // Test serial chip (send byte 0xAE and check if serial returns same byte)
    outb(SERIAL_DATA_PORT(SERIAL_COM1_PORT), SERIAL_TEST_PAYLOAD);

    // Check if serial is faulty (i.e: not same byte as sent)
    if(inb(SERIAL_DATA_PORT(SERIAL_COM1_PORT)) != SERIAL_TEST_PAYLOAD) {
        return -1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(SERIAL_MODEM_COMMAND_PORT(SERIAL_COM1_PORT), SERIAL_MODEM_NORMAL_OP);

    // Initialize spinlock for printf
    init_spinlock(&serial_printf_spinlock, "serial_printf spinlock");

    // Mark serial as initialized
    serial_initialized = 1;
    serial_printf("[SERIAL] Serial port COM1 initialized successfully\r\n");

    return 0;
}

int serial_is_transmit_empty() {
    return inb(SERIAL_LINE_STATUS_PORT(SERIAL_COM1_PORT)) & SERIAL_TRANSMIT_EMPTY_MASK;
}

void serial_putchar(char c) {
    if (!serial_initialized) return;

    // Wait for transmit buffer to be empty
    while (serial_is_transmit_empty() == 0);

    outb(SERIAL_DATA_PORT(SERIAL_COM1_PORT), c);
}

void serial_write(const char *str) {
    if (!serial_initialized) return;

    while (*str != 0) {
        serial_putchar(*str++);
    }
}


void serial_printf(const char *format, ...) {
    if (!serial_initialized) return;

    acquire_spinlock(&serial_printf_spinlock);
    va_list varargs;
    va_start(varargs, format);

    char digits_buf[MAX_DIGIT_BUFFER_SIZE];
    memset(digits_buf, 0, MAX_DIGIT_BUFFER_SIZE);

    while (*format) {
        switch (*format) {
            case '%':
                format++;
                switch (*format) {
                    case 'd':
                        if (itoa(va_arg(varargs, int), digits_buf, 10) == 0) {
                            serial_write(digits_buf);
                        } else {
                            serial_putchar('#');
                        }
                        break;
                    case 'o':
                        if (itoa(va_arg(varargs, int), digits_buf, 8) == 0) {
                            serial_write(digits_buf);
                        } else {
                            serial_putchar('#');
                        }
                        break;
                    case 'x':
                        if (itoa(va_arg(varargs, int), digits_buf, 16) == 0) {
                            serial_write(digits_buf);
                        } else {
                            serial_putchar('#');
                        }
                        break;
                    case 'b':
                        if (itoa(va_arg(varargs, int), digits_buf, 2) == 0) {
                            serial_write(digits_buf);
                        } else {
                            serial_putchar('#');
                        }
                        break;
                    case 'p':
                        if (ptoa(va_arg(varargs, uint64_t), digits_buf) == 0) {
                            serial_write(digits_buf);
                        } else {
                            serial_putchar('#');
                        }
                        serial_write(digits_buf);
                        break;
                    case 's':
                        serial_write(va_arg(varargs, char*));
                        break;
                    case '%':
                        serial_putchar('%');
                        break;
                    default:
                        serial_putchar('#');
                }
                break;
            default:
                serial_putchar(*format);
        }
        format++;
    }
    va_end(varargs);
    release_spinlock(&serial_printf_spinlock);
}