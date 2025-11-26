//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// VGA text-mode definitions for ShipOS kernel.
// Provides constants, types, and function declarations for
// working with the VGA text buffer.
//

#ifndef UNTITLED_OS_PRINT_H
#define UNTITLED_OS_PRINT_H

/**
 * @brief VGA screen dimensions
 */
#define VGA_WIDTH 80  // Number of columns
#define VGA_HEIGHT 25 // Number of rows

/**
 * @brief Default VGA color used for initialization
 */
#define VGA_COLOR 7

#include <inttypes.h>

/**
 * @brief Default background and foreground colors
 */
#define DEFAULT_BG_COLOR VGA_COLOR_WHITE
#define DEFAULT_FG_COLOR VGA_COLOR_BLACK

/**
 * @brief VGA color palette.
 * Each color corresponds to a 4-bit code used in VGA text mode.
 */
enum vga_colors
{
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GRAY = 7,
    VGA_COLOR_DARK_GRAY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15
};

/**
 * @brief Structure representing a character with a color attribute.
 *
 * Packed to ensure exactly 2 bytes per character cell in VGA memory:
 * - character: ASCII code of the character
 * - color: foreground/background color code
 */
struct __attribute__((packed)) char_with_color
{
    uint8_t character;
    uint8_t color;
};

/**
 * @brief Clears the VGA screen.
 *
 * Resets all characters on the screen to zero and moves the cursor to the top-left.
 */
void clear_vga();

/**
 * @brief Writes a full buffer to the VGA screen.
 *
 * Copies the content of tty_buffer into VGA memory. The buffer
 * should be VGA_HEIGHT * VGA_WIDTH in size.
 *
 * @param tty_buffer Pointer to the buffer to write to VGA memory
 */
void write_buffer(struct char_with_color *tty_buffer);

#endif // UNTITLED_OS_PRINT_H
