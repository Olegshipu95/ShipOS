//
// Created by ShipOS developers on 22.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//
// Serial port driver implementation for kernel debugging and logging.
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
 * @brief Standard COM port addresses to probe
 */
static const uint16_t standard_com_ports[MAX_SERIAL_PORTS] = {
    SERIAL_COM1_PORT,
    SERIAL_COM2_PORT,
    SERIAL_COM3_PORT,
    SERIAL_COM4_PORT
};

/**
 * @brief Array of initialized ports
 */
static uint16_t initialized_ports[MAX_SERIAL_PORTS] = {0};
static int num_initialized_ports = 0;

/**
 * @brief Default serial port (initially COM1)
 */
static uint16_t default_serial_port = SERIAL_COM1_PORT;

/**
 * @brief Per-port spinlocks for thread-safe output
 * Using an array to support multiple ports
 */
static struct spinlock serial_printf_spinlocks[MAX_SERIAL_PORTS];

/**
 * @brief Find the index of a port in the initialized_ports array
 * @param port Port address
 * @return Index if found, -1 otherwise
 */
static int get_port_index(uint16_t port) {
    for (int i = 0; i < num_initialized_ports; i++) {
        if (initialized_ports[i] == port) {
            return i;
        }
    }
    return -1;
}

int detect_serial_ports(uint16_t *ports) {
    int detected = 0;
    for (int i = 0; i < MAX_SERIAL_PORTS; i++) {
        uint16_t port = standard_com_ports[i];
        if (init_serial(port) == 0) {
            ports[detected++] = port;
        }
    }
    return detected;
}

int init_serial(uint16_t port) {
    if (port == 0) return -1;

    // Disable all interrupts
    outb(port + 1, SERIAL_DISABLE_INTERRUPTS);

    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_LINE_COMMAND_PORT(port), SERIAL_ENABLE_DLAB);

    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_DATA_PORT(port), SERIAL_BAUD_DIVISOR_LOW);

    // Set divisor to 3 (hi byte)
    outb(port + 1, SERIAL_BAUD_DIVISOR_HIGH);

    // 8 bits, no parity, one stop bit
    outb(SERIAL_LINE_COMMAND_PORT(port), SERIAL_LINE_CONFIG_8N1);

    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_FIFO_COMMAND_PORT(port), SERIAL_FIFO_ENABLE_CLEAR_14B);

    // IRQs enabled, RTS/DSR set
    outb(SERIAL_MODEM_COMMAND_PORT(port), SERIAL_MODEM_IRQ_RTS_DSR);

    // Set in loopback mode, test the serial chip
    outb(SERIAL_MODEM_COMMAND_PORT(port), SERIAL_MODEM_LOOPBACK);

    // Test serial chip (send byte 0xAE and check if serial returns same byte)
    outb(SERIAL_DATA_PORT(port), SERIAL_TEST_PAYLOAD);

    // Check if serial is faulty (i.e: not same byte as sent)
    if (inb(SERIAL_DATA_PORT(port)) != SERIAL_TEST_PAYLOAD) {
        return -1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(SERIAL_MODEM_COMMAND_PORT(port), SERIAL_MODEM_NORMAL_OP);

    // Add to initialized ports if not already present
    int index = get_port_index(port);
    if (index == -1) {
        if (num_initialized_ports >= MAX_SERIAL_PORTS) {
            return -1; // No more slots
        }
        index = num_initialized_ports++;
        initialized_ports[index] = port;
    }

    // Initialize spinlock for this port
    char lock_name[32];
    // snprintf(lock_name, sizeof(lock_name), "serial_printf_%x", port);
    init_spinlock(&serial_printf_spinlocks[index], lock_name);

    // Log success using this port
    serial_printf(port, "[SERIAL] Serial port at 0x%x initialized successfully\r\n", port);

    // If first port, set as default
    if (num_initialized_ports == 1) {
        set_default_serial_port(port);
    }

    return 0;
}

void set_default_serial_port(uint16_t port) {
    if (get_port_index(port) != -1) {
        default_serial_port = port;
    }
}

uint16_t get_default_serial_port() {
    return default_serial_port;
}

int serial_is_transmit_empty(uint16_t port) {
    if (get_port_index(port) == -1) return 0;
    return inb(SERIAL_LINE_STATUS_PORT(port)) & SERIAL_TRANSMIT_EMPTY_MASK;
}

void serial_putchar(uint16_t port, char c) {
    if (get_port_index(port) == -1) return;

    // Wait for transmit buffer to be empty
    while (serial_is_transmit_empty(port) == 0);

    outb(SERIAL_DATA_PORT(port), c);
}

void serial_write(uint16_t port, const char *str) {
    if (get_port_index(port) == -1) return;

    while (*str != 0) {
        serial_putchar(port, *str++);
    }
}

void serial_printf(uint16_t port, const char *format, ...) {
    int index = get_port_index(port);
    if (index == -1) return;

    acquire_spinlock(&serial_printf_spinlocks[index]);
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
                            serial_write(port, digits_buf);
                        } else {
                            serial_putchar(port, '#');
                        }
                        break;
                    case 'o':
                        if (itoa(va_arg(varargs, int), digits_buf, 8) == 0) {
                            serial_write(port, digits_buf);
                        } else {
                            serial_putchar(port, '#');
                        }
                        break;
                    case 'x':
                        if (itoa(va_arg(varargs, int), digits_buf, 16) == 0) {
                            serial_write(port, digits_buf);
                        } else {
                            serial_putchar(port, '#');
                        }
                        break;
                    case 'b':
                        if (itoa(va_arg(varargs, int), digits_buf, 2) == 0) {
                            serial_write(port, digits_buf);
                        } else {
                            serial_putchar(port, '#');
                        }
                        break;
                    case 'p':
                        if (ptoa(va_arg(varargs, uint64_t), digits_buf) == 0) {
                            serial_write(port, digits_buf);
                        } else {
                            serial_putchar(port, '#');
                        }
                        break;
                    case 's':
                        serial_write(port, va_arg(varargs, char*));
                        break;
                    case '%':
                        serial_putchar(port, '%');
                        break;
                    default:
                        serial_putchar(port, '#');
                }
                break;
            default:
                serial_putchar(port, *format);
        }
        format++;
    }
    va_end(varargs);
    release_spinlock(&serial_printf_spinlocks[index]);
}
