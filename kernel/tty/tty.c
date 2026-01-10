//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// Terminal (TTY) handling for ShipOS kernel.
// Provides multiple virtual terminals, text output, and
// formatted printing to VGA memory.
//

#include <stdarg.h>
#include "tty.h"
#include <inttypes.h>
#include "../lib/include/logging.h"
#include "../lib/include/memset.h"
#include "../lib/include/str_utils.h"

/**
 * @brief Maximum number of virtual terminals
 */
#define TERMINALS_NUMBER 7

/**
 * @brief Array of virtual terminal structures
 */
static tty_structure tty_terminals[TERMINALS_NUMBER];

/**
 * @brief Pointer to the currently active terminal
 */
static tty_structure *active_tty;

/**
 * @brief Spinlocks to make printf and print functions thread-safe
 */
static struct spinlock printf_spinlock;
static struct spinlock print_spinlock;

void set_fg(enum vga_colors _fg) {
    active_tty->fg = _fg;
}

void set_bg(enum vga_colors _bg) {
    active_tty->bg = _bg;
}

void init_tty() {
    memset(&tty_terminals, 0, sizeof(tty_structure) * TERMINALS_NUMBER);
    for (int i = 0; i < TERMINALS_NUMBER; ++i) {
        (tty_terminals + i)->tty_id = i;
        (tty_terminals + i)->bg = DEFAULT_BG_COLOR;
        (tty_terminals + i)->fg = DEFAULT_FG_COLOR;
        (tty_terminals + i)->line = (tty_terminals + i)->pos = 0;
        memset((tty_terminals + i)->tty_buffer, 0, VGA_HEIGHT * VGA_WIDTH);
    }
    set_tty(0);
    init_spinlock(&printf_spinlock, "printf spinlock");
    init_spinlock(&print_spinlock, "print spinlock");
    LOG("TTY subsystem initialized");
}

void set_tty(uint8_t terminal) {
    if (TERMINALS_NUMBER <= terminal) {
        return;
    }
    clear_vga();
    active_tty = tty_terminals + terminal;
    LOG("TTY %d", active_tty->tty_id);
    write_buffer(active_tty->tty_buffer);
}

void clear_current_tty() {
    memset(active_tty->tty_buffer, 0, VGA_WIDTH * VGA_HEIGHT);
    active_tty->pos = 0;
    active_tty->line = 0;
    clear_vga();
}

uint8_t get_current_tty() {
    return active_tty->tty_id;
}

/**
 * @brief Construct a VGA character with foreground and background colors
 * @param value ASCII character
 * @param fg Foreground color
 * @param bg Background color
 * @return char_with_color structure
 */
struct char_with_color make_char(char value, enum vga_colors fg, enum vga_colors bg) {
    struct char_with_color res = {
            .character = value,
            .color = fg + (bg << 4)
    };
    return res;
}

/**
 * @brief Scroll the terminal buffer up by one line
 */
void scroll() {
    for (int i = 1; i < VGA_HEIGHT; i++) {
        for (int j = 0; j < VGA_WIDTH; j++) {
            active_tty->tty_buffer[(i - 1) * VGA_WIDTH + j] = active_tty->tty_buffer[i * VGA_WIDTH + j];
        }
    }
    for (int i = 0; i < VGA_WIDTH; i++) {
        active_tty->tty_buffer[VGA_WIDTH * (VGA_HEIGHT - 1) + i] = make_char(0, 0, 0);
    }
    active_tty->line = VGA_HEIGHT - 1;
    active_tty->pos = 0;
}

/**
 * @brief Put a single character on the terminal, handling newlines and scrolling
 * @param c Pointer to character
 */
void putchar(char *c) {
    if (active_tty->line >= VGA_HEIGHT) scroll();

    if (*c == '\n') {
        active_tty->line++;
        active_tty->pos = 0;
    } else {
        *(active_tty->tty_buffer + active_tty->line * VGA_WIDTH + active_tty->pos) =
            make_char(*c, active_tty->fg, active_tty->bg);
        active_tty->pos += 1;
        active_tty->line += active_tty->pos / VGA_WIDTH;
        active_tty->pos %= VGA_WIDTH;
    }
}

void print(const char *string) {
    acquire_spinlock(&print_spinlock);
    while (*string != 0) {
        putchar((char *)string++);
    }
    write_buffer(active_tty->tty_buffer);
    release_spinlock(&print_spinlock);
}

void printf(const char *format, ...) {
    acquire_spinlock(&printf_spinlock);
    va_list varargs;
    va_start(varargs, format);

    char buf[256];  // Output buffer for formatted values

    while (*format) {
        if (*format != '%') {
            putchar((char *)format);
            write_buffer(active_tty->tty_buffer);
            format++;
            continue;
        }
        
        format++;  // Skip '%'
        
        // Check for %%
        if (*format == '%') {
            putchar("%");
            write_buffer(active_tty->tty_buffer);
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
                print(buf);
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
                print(buf);
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
                print(buf);
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
                print(buf);
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
                print(buf);
                break;
            }
            
            case 'p': {
                uint64_t val = (uint64_t)(uintptr_t)va_arg(varargs, void *);
                spec.flags |= FMT_FLAG_HASH;
                if (spec.precision < 0) spec.precision = 1;
                format_unsigned(val, buf, 16, &spec);
                print(buf);
                break;
            }
            
            case 's': {
                char *str = va_arg(varargs, char *);
                format_string(str, buf, sizeof(buf), &spec);
                print(buf);
                break;
            }
            
            case 'c': {
                char c[2] = {(char)va_arg(varargs, int), '\0'};
                if (spec.width > 1 && !(spec.flags & FMT_FLAG_LEFT)) {
                    for (int i = 0; i < spec.width - 1; i++) {
                        putchar(" ");
                        write_buffer(active_tty->tty_buffer);
                    }
                }
                print(c);
                if (spec.width > 1 && (spec.flags & FMT_FLAG_LEFT)) {
                    for (int i = 0; i < spec.width - 1; i++) {
                        putchar(" ");
                        write_buffer(active_tty->tty_buffer);
                    }
                }
                break;
            }
            
            case 'n': {
                // Not supported for safety
                break;
            }
            
            default:
                putchar("%");
                write_buffer(active_tty->tty_buffer);
                if (*format) {
                    char unknown[2] = {*format, '\0'};
                    print(unknown);
                }
                break;
        }
        
        if (*format) format++;
    }

    release_spinlock(&printf_spinlock);
}

