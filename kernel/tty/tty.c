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
}

void set_tty(uint8_t terminal) {
    if (TERMINALS_NUMBER <= terminal) {
        return;
    }
    clear_vga();
    active_tty = tty_terminals + terminal;
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

    char digits_buf[MAX_DIGIT_BUFFER_SIZE];
    memset(digits_buf, 0, MAX_DIGIT_BUFFER_SIZE);

    while (*format) {
        switch (*format) {
            case '%':
                format++;
                switch (*format) {
                    case 'd':
                        if (itoa(va_arg(varargs, int), digits_buf, 10) == 0) {
                            print(digits_buf);
                        } else {
                            putchar("#");
                        }
                        break;
                    case 'o':
                        if (itoa(va_arg(varargs, int), digits_buf, 8) == 0) {
                            print(digits_buf);
                        } else {
                            putchar("#");
                        }
                        break;
                    case 'x':
                        if (itoa(va_arg(varargs, int), digits_buf, 16) == 0) {
                            print(digits_buf);
                        } else {
                            putchar("#");
                        }
                        break;
                    case 'b':
                        if (itoa(va_arg(varargs, int), digits_buf, 2) == 0) {
                            print(digits_buf);
                        } else {
                            putchar("#");
                        }
                        break;
                    case 'p':
                        if (ptoa(va_arg(varargs, uint64_t), digits_buf) == 0) {
                            print(digits_buf);
                        } else {
                            putchar("#");
                        }
                        break;
                    case 's':
                        print(va_arg(varargs, char*));
                        break;
                    case '%':
                        putchar("%");
                        break;
                    default:
                        putchar("#");
                        write_buffer(active_tty->tty_buffer);
                }
                break;
            default:
                putchar((char *)format);
                write_buffer(active_tty->tty_buffer);
        }
        format++;
    }

    release_spinlock(&printf_spinlock);
}

