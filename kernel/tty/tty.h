//
// Created by untitled-os developers on 28.10.23.
// Copyright (c) 2023 untitled-os. All rights reserved.
//
// Terminal (TTY) interface for untitled-os kernel.
// Provides multiple virtual terminals, text output, and formatted printing.
//

#ifndef UNTITLED_OS_TTY_H
#define UNTITLED_OS_TTY_H

#include "../vga/vga.h"
#include <inttypes.h>
#include "../sync/spinlock.h"
#include "../lib/include/x86_64.h"

/**
 * @brief Maximum number of virtual terminals
 */
#define TERMINALS_NUMBER 7

/**
 * @brief Structure representing a single virtual terminal
 */
typedef struct {
    struct char_with_color tty_buffer[VGA_HEIGHT * VGA_WIDTH]; //< Terminal screen buffer
    uint8_t tty_id;    //< Terminal index
    uint8_t line;      //< Current cursor line
    uint8_t pos;       //< Current cursor position within line
    enum vga_colors bg; //< Current background color
    enum vga_colors fg; //< Current foreground color
} tty_structure;

/**
 * @brief Initialize all virtual terminals.
 * Clears buffers, sets default colors, and initializes spinlocks.
 */
void init_tty();

/**
 * @brief Activate a terminal by index and write its buffer to VGA
 * @param terminal Terminal number
 */
void set_tty(uint8_t terminal);

/**
 * @brief Set foreground color for current terminal
 * @param _fg VGA color code
 */
void set_fg(enum vga_colors fg);

/**
 * @brief Set background color for current terminal
 * @param _bg VGA color code
 */
void set_bg(enum vga_colors bg);

/**
 * @brief Print a null-terminated string to the currently active terminal
 * @param string String to print
 */
void print(const char *string);

/**
 * @brief Formatted print to terminal (supports %d, %x, %o, %b, %p, %s, %%)
 * Uses spinlock to prevent concurrent writes
 * @param format Format string
 * @param ... Variadic arguments
 */
void printf(const char* format, ...);

/**
 * @brief Clear the buffer of the currently active terminal
 */
void clear_current_tty();

/**
 * @brief Get the index of the currently active terminal
 * @return Terminal number
 */
uint8_t get_current_tty();

#endif //UNTITLED_OS_TTY_H

