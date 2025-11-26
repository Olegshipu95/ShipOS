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

void set_fg(enum vga_colors _fg)
{
    active_tty->fg = _fg;
}

void set_bg(enum vga_colors _bg)
{
    active_tty->bg = _bg;
}

void init_tty()
{
    memset(&tty_terminals, 0, sizeof(tty_structure) * TERMINALS_NUMBER);
    for (int i = 0; i < TERMINALS_NUMBER; ++i)
    {
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

void set_tty(uint8_t terminal)
{
    if (TERMINALS_NUMBER <= terminal)
    {
        return;
    }
    clear_vga();
    active_tty = tty_terminals + terminal;
    write_buffer(active_tty->tty_buffer);
}

void clear_current_tty()
{
    memset(active_tty->tty_buffer, 0, VGA_WIDTH * VGA_HEIGHT);
    active_tty->pos = 0;
    active_tty->line = 0;
    clear_vga();
}

uint8_t
get_current_tty()
{
    return active_tty->tty_id;
}

/**
 * @brief Reverse a string in-place
 * @param str String to reverse
 * @param n Length of the string
 */
void reverse(char *str, int n)
{
    int i = 0;
    int j = n - 1;
    while (i < j)
    {
        char tmp = str[i];
        str[i++] = str[j];
        str[j--] = tmp;
    }
}

/**
 * @brief Construct a VGA character with foreground and background colors
 * @param value ASCII character
 * @param fg Foreground color
 * @param bg Background color
 * @return char_with_color structure
 */
struct char_with_color
make_char(char value, enum vga_colors fg, enum vga_colors bg)
{
    struct char_with_color res = {
        .character = value,
        .color = fg + (bg << 4)};
    return res;
}

/**
 * @brief Scroll the terminal buffer up by one line
 */
void scroll()
{
    for (int i = 1; i < VGA_HEIGHT; i++)
    {
        for (int j = 0; j < VGA_WIDTH; j++)
        {
            active_tty->tty_buffer[(i - 1) * VGA_WIDTH + j] = active_tty->tty_buffer[i * VGA_WIDTH + j];
        }
    }
    for (int i = 0; i < VGA_WIDTH; i++)
    {
        active_tty->tty_buffer[VGA_WIDTH * (VGA_HEIGHT - 1) + i] = make_char(0, 0, 0);
    }
    active_tty->line = VGA_HEIGHT - 1;
    active_tty->pos = 0;
}

/**
 * @brief Put a single character on the terminal, handling newlines and scrolling
 * @param c Pointer to character
 */
void putchar(char *c)
{
    if (active_tty->line >= VGA_HEIGHT)
        scroll();

    if (*c == '\n')
    {
        active_tty->line++;
        active_tty->pos = 0;
    }
    else
    {
        *(active_tty->tty_buffer + active_tty->line * VGA_WIDTH + active_tty->pos) = make_char(*c, active_tty->fg, active_tty->bg);
        active_tty->pos += 1;
        active_tty->line += active_tty->pos / VGA_WIDTH;
        active_tty->pos %= VGA_WIDTH;
    }
}

void print(const char *string)
{
    acquire_spinlock(&print_spinlock);
    while (*string != 0)
    {
        putchar((char *) string++);
    }
    write_buffer(active_tty->tty_buffer);
    release_spinlock(&print_spinlock);
}

void itoa(int num, char *str, int radix)
{
    int i = 0;
    int is_negative = 0;
    if (num < 0 && radix != 16)
    {
        is_negative = 1;
        num *= -1;
    }

    do
    {
        int rem = (num % radix);
        str[i++] = (rem > 9 ? 'a' - 10 : '0') + rem;
        num /= radix;
    } while (num);

    if (is_negative)
        str[i++] = '-';
    reverse(str, i);
    str[i] = 0;
}

/**
 * @brief Convert 64-bit integer to hexadecimal string
 * @param num 64-bit number
 * @param str Output buffer
 */
void ptoa(uint64_t num, char *str)
{
    int i = 0;

    do
    {
        int rem = (num % 16);
        str[i++] = (rem > 9 ? 'a' - 10 : '0') + rem;
        num /= 16;
    } while (num);

    reverse(str, i);
    str[i] = 0;
}

void printf(const char *format, ...)
{
    acquire_spinlock(&printf_spinlock);
    va_list varargs;
    va_start(varargs, format);
    char digits_buf[100];
    for (int i = 0; i < 100; i++)
        digits_buf[i] = 0;

    while (*format)
    {
        switch (*format)
        {
        case '%':
            format++;
            switch (*format)
            {
            case 'd':
                itoa(va_arg(varargs, int), digits_buf, 10);
                print(digits_buf);
                break;
            case 'o':
                itoa(va_arg(varargs, int), digits_buf, 8);
                print(digits_buf);
                break;
            case 'x':
                itoa(va_arg(varargs, int), digits_buf, 16);
                print(digits_buf);
                break;
            case 'b':
                itoa(va_arg(varargs, int), digits_buf, 2);
                print(digits_buf);
                break;
            case 'p':
                ptoa(va_arg(varargs, uint64_t), digits_buf);
                print(digits_buf);
                break;
            case 's':
                print(va_arg(varargs, char *));
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
            putchar((char *) format);
            write_buffer(active_tty->tty_buffer);
        }
        format++;
    }

    release_spinlock(&printf_spinlock);
}
