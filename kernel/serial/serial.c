//
// Created by ShipOS developers on 22.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//
// Serial port (COM1) driver implementation for kernel debugging and logging.
//

#include "serial.h"
#include <stdarg.h>
#include "../lib/include/x86_64.h"
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
 * @brief Spinlock for serial_printf to ensure thread-safe output
 */
static struct spinlock serial_printf_spinlock;

/**
 * @brief Flag indicating if serial port is initialized
 */
static int serial_initialized = 0;

int init_serial(int32_t port) {
    // Disable all interrupts
    outb(SERIAL_COM1_PORT + 1, 0x00);

    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_LINE_COMMAND_PORT(SERIAL_COM1_PORT), 0x80);

    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_DATA_PORT(SERIAL_COM1_PORT), 0x03);

    // Set divisor to 3 (hi byte)
    outb(SERIAL_COM1_PORT + 1, 0x00);

    // 8 bits, no parity, one stop bit
    outb(SERIAL_LINE_COMMAND_PORT(SERIAL_COM1_PORT), 0x03);

    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_FIFO_COMMAND_PORT(SERIAL_COM1_PORT), 0xC7);

    // IRQs enabled, RTS/DSR set
    outb(SERIAL_MODEM_COMMAND_PORT(SERIAL_COM1_PORT), 0x0B);

    // Set in loopback mode, test the serial chip
    outb(SERIAL_MODEM_COMMAND_PORT(SERIAL_COM1_PORT), 0x1E);

    // Test serial chip (send byte 0xAE and check if serial returns same byte)
    outb(SERIAL_DATA_PORT(SERIAL_COM1_PORT), 0xAE);

    // Check if serial is faulty (i.e: not same byte as sent)
    if(inb(SERIAL_DATA_PORT(SERIAL_COM1_PORT)) != 0xAE) {
        return -1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(SERIAL_MODEM_COMMAND_PORT(SERIAL_COM1_PORT), 0x0F);

    // Initialize spinlock for printf
    init_spinlock(&serial_printf_spinlock, "serial_printf spinlock");

    // Mark serial as initialized
    serial_initialized = 1;
    serial_printf("[SERIAL] Serial port COM1 initialized successfully\r\n");

    return 0;
}

int serial_is_transmit_empty() {
    return inb(SERIAL_LINE_STATUS_PORT(SERIAL_COM1_PORT)) & 0x20;
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

/**
 * @brief Reverse a string in-place
 * @param str String to reverse
 * @param n Length of the string
 */
static void reverse_str(char *str, int n) {
    int i = 0;
    int j = n - 1;
    while (i < j) {
        char tmp = str[i];
        str[i++] = str[j];
        str[j--] = tmp;
    }
}

/**
 * @brief Convert integer to string with specified radix
 * @param num Integer value
 * @param str Output buffer
 * @param radix Base (e.g., 10, 16, 2)
 */
static void serial_itoa(int num, char *str, int radix) {
    int i = 0;
    int is_negative = 0;
    if (num < 0 && radix != 16) {
        is_negative = 1;
        num *= -1;
    }

    do {
        int rem = (num % radix);
        str[i++] = (rem > 9 ? 'a' - 10 : '0') + rem;
        num /= radix;
    } while (num);

    if (is_negative) str[i++] = '-';
    reverse_str(str, i);
    str[i] = 0;
}

/**
 * @brief Convert 64-bit integer to hexadecimal string
 * @param num 64-bit number
 * @param str Output buffer
 */
static void serial_ptoa(uint64_t num, char *str) {
    int i = 0;

    do {
        int rem = (num % 16);
        str[i++] = (rem > 9 ? 'a' - 10 : '0') + rem;
        num /= 16;
    } while (num);

    reverse_str(str, i);
    str[i] = 0;
}

void serial_printf(const char *format, ...) {
    if (!serial_initialized) return;

    acquire_spinlock(&serial_printf_spinlock);
    va_list varargs;
    va_start(varargs, format);

    const uint8_t BUFFER_SIZE = 255;
    char digits_buf[BUFFER_SIZE];
    memset(digits_buf, 0, BUFFER_SIZE);

    while (*format) {
        switch (*format) {
            case '%':
                format++;
                switch (*format) {
                    case 'd':
                        serial_itoa(va_arg(varargs, int), digits_buf, 10);
                        serial_write(digits_buf);
                        break;
                    case 'o':
                        serial_itoa(va_arg(varargs, int), digits_buf, 8);
                        serial_write(digits_buf);
                        break;
                    case 'x':
                        serial_itoa(va_arg(varargs, int), digits_buf, 16);
                        serial_write(digits_buf);
                        break;
                    case 'b':
                        serial_itoa(va_arg(varargs, int), digits_buf, 2);
                        serial_write(digits_buf);
                        break;
                    case 'p':
                        serial_ptoa(va_arg(varargs, uint64_t), digits_buf);
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

    release_spinlock(&serial_printf_spinlock);
}
