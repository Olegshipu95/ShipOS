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
 * @brief Standard spinlock names
 */
static const char* serial_printf_spinlock_names[MAX_SERIAL_PORTS] = {
    "serial_printf_1",
    "serial_printf_2",
    "serial_printf_3",
    "serial_printf_4",
};

/**
 * @brief Array of initialized ports
 */
static uint16_t initialized_ports[MAX_SERIAL_PORTS] = {0};
static int num_initialized_ports = -1;

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

int init_serial_ports() {
    int serial_ports_count = detect_serial_ports();
    if (serial_ports_count <= 0) {
        return -1;
    }
    int used_port = serial_ports_count - 1;
    set_default_serial_port(initialized_ports[used_port]);
    return serial_ports_count;
}

int detect_serial_ports() {
    if (num_initialized_ports != -1) {
        return num_initialized_ports;
    }

    num_initialized_ports = 0;
    for (int i = 0; i < MAX_SERIAL_PORTS; i++) {
        uint16_t port = standard_com_ports[i];
        if (init_serial(port) == 0) {
            initialized_ports[num_initialized_ports++] = port;
        }
    }
    return num_initialized_ports;
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
    init_spinlock(&serial_printf_spinlocks[index], serial_printf_spinlock_names[index]);

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

    char buf[256];  // Output buffer for formatted values

    while (*format) {
        if (*format != '%') {
            serial_putchar(port, *format++);
            continue;
        }
        
        format++;  // Skip '%'
        
        // Check for %%
        if (*format == '%') {
            serial_putchar(port, '%');
            format++;
            continue;
        }
        
        // Parse format specification
        struct fmt_spec spec = {0, -1, -1, FMT_LEN_DEFAULT};
        
        // Parse flags: -, +, space, #, 0
        int parsing_flags = 1;
        while (parsing_flags && *format) {
            switch (*format) {
                case '-': spec.flags |= FMT_FLAG_LEFT; format++; break;
                case '+': spec.flags |= FMT_FLAG_PLUS; format++; break;
                case ' ': spec.flags |= FMT_FLAG_SPACE; format++; break;
                case '#': spec.flags |= FMT_FLAG_HASH; format++; break;
                case '0': spec.flags |= FMT_FLAG_ZERO; format++; break;
                default: parsing_flags = 0; break;
            }
        }
        
        // Parse width
        if (*format == '*') {
            spec.width = va_arg(varargs, int);
            if (spec.width < 0) {
                spec.flags |= FMT_FLAG_LEFT;
                spec.width = -spec.width;
            }
            format++;
        } else {
            spec.width = 0;
            while (*format >= '0' && *format <= '9') {
                spec.width = spec.width * 10 + (*format - '0');
                format++;
            }
            if (spec.width == 0) spec.width = -1;  // Not specified
        }
        
        // Parse precision
        if (*format == '.') {
            format++;
            if (*format == '*') {
                spec.precision = va_arg(varargs, int);
                if (spec.precision < 0) spec.precision = -1;
                format++;
            } else {
                spec.precision = 0;
                while (*format >= '0' && *format <= '9') {
                    spec.precision = spec.precision * 10 + (*format - '0');
                    format++;
                }
            }
        }
        
        // Parse length modifier
        if (*format == 'h') {
            format++;
            if (*format == 'h') {
                spec.length = FMT_LEN_HH;
                format++;
            } else {
                spec.length = FMT_LEN_H;
            }
        } else if (*format == 'l') {
            format++;
            if (*format == 'l') {
                spec.length = FMT_LEN_LL;
                format++;
            } else {
                spec.length = FMT_LEN_L;
            }
        } else if (*format == 'z') {
            spec.length = FMT_LEN_Z;
            format++;
        }
        
        // Parse conversion specifier
        switch (*format) {
            case 'd':
            case 'i': {
                int64_t val;
                switch (spec.length) {
                    case FMT_LEN_HH: val = (signed char)va_arg(varargs, int); break;
                    case FMT_LEN_H:  val = (short)va_arg(varargs, int); break;
                    case FMT_LEN_L:  val = va_arg(varargs, long); break;
                    case FMT_LEN_LL: val = va_arg(varargs, long long); break;
                    case FMT_LEN_Z:  val = va_arg(varargs, size_t); break;
                    default:         val = va_arg(varargs, int); break;
                }
                format_signed(val, buf, 10, &spec);
                serial_write(port, buf);
                break;
            }
            
            case 'u': {
                uint64_t val;
                switch (spec.length) {
                    case FMT_LEN_HH: val = (unsigned char)va_arg(varargs, unsigned int); break;
                    case FMT_LEN_H:  val = (unsigned short)va_arg(varargs, unsigned int); break;
                    case FMT_LEN_L:  val = va_arg(varargs, unsigned long); break;
                    case FMT_LEN_LL: val = va_arg(varargs, unsigned long long); break;
                    case FMT_LEN_Z:  val = va_arg(varargs, size_t); break;
                    default:         val = va_arg(varargs, unsigned int); break;
                }
                format_unsigned(val, buf, 10, &spec);
                serial_write(port, buf);
                break;
            }
            
            case 'x':
            case 'X': {
                if (*format == 'X') spec.flags |= FMT_FLAG_UPPER;
                uint64_t val;
                switch (spec.length) {
                    case FMT_LEN_HH: val = (unsigned char)va_arg(varargs, unsigned int); break;
                    case FMT_LEN_H:  val = (unsigned short)va_arg(varargs, unsigned int); break;
                    case FMT_LEN_L:  val = va_arg(varargs, unsigned long); break;
                    case FMT_LEN_LL: val = va_arg(varargs, unsigned long long); break;
                    case FMT_LEN_Z:  val = va_arg(varargs, size_t); break;
                    default:         val = va_arg(varargs, unsigned int); break;
                }
                format_unsigned(val, buf, 16, &spec);
                serial_write(port, buf);
                break;
            }
            
            case 'o': {
                uint64_t val;
                switch (spec.length) {
                    case FMT_LEN_HH: val = (unsigned char)va_arg(varargs, unsigned int); break;
                    case FMT_LEN_H:  val = (unsigned short)va_arg(varargs, unsigned int); break;
                    case FMT_LEN_L:  val = va_arg(varargs, unsigned long); break;
                    case FMT_LEN_LL: val = va_arg(varargs, unsigned long long); break;
                    case FMT_LEN_Z:  val = va_arg(varargs, size_t); break;
                    default:         val = va_arg(varargs, unsigned int); break;
                }
                format_unsigned(val, buf, 8, &spec);
                serial_write(port, buf);
                break;
            }
            
            case 'b': {
                uint64_t val;
                switch (spec.length) {
                    case FMT_LEN_HH: val = (unsigned char)va_arg(varargs, unsigned int); break;
                    case FMT_LEN_H:  val = (unsigned short)va_arg(varargs, unsigned int); break;
                    case FMT_LEN_L:  val = va_arg(varargs, unsigned long); break;
                    case FMT_LEN_LL: val = va_arg(varargs, unsigned long long); break;
                    case FMT_LEN_Z:  val = va_arg(varargs, size_t); break;
                    default:         val = va_arg(varargs, unsigned int); break;
                }
                format_unsigned(val, buf, 2, &spec);
                serial_write(port, buf);
                break;
            }
            
            case 'p': {
                uint64_t val = (uint64_t)(uintptr_t)va_arg(varargs, void *);
                spec.flags |= FMT_FLAG_HASH;
                if (spec.precision < 0) spec.precision = 1;
                format_unsigned(val, buf, 16, &spec);
                serial_write(port, buf);
                break;
            }
            
            case 's': {
                char *str = va_arg(varargs, char *);
                format_string(str, buf, sizeof(buf), &spec);
                serial_write(port, buf);
                break;
            }
            
            case 'c': {
                char c = (char)va_arg(varargs, int);
                if (spec.width > 1 && !(spec.flags & FMT_FLAG_LEFT)) {
                    for (int i = 0; i < spec.width - 1; i++) {
                        serial_putchar(port, ' ');
                    }
                }
                serial_putchar(port, c);
                if (spec.width > 1 && (spec.flags & FMT_FLAG_LEFT)) {
                    for (int i = 0; i < spec.width - 1; i++) {
                        serial_putchar(port, ' ');
                    }
                }
                break;
            }
            
            case 'n': {
                // Not supported for safety
                break;
            }
            
            default:
                serial_putchar(port, '%');
                if (*format) serial_putchar(port, *format);
                break;
        }
        
        if (*format) format++;
    }
    
    va_end(varargs);
    release_spinlock(&serial_printf_spinlocks[index]);
}
