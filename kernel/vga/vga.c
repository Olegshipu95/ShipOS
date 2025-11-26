//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// VGA text-mode driver for ShipOS kernel.
// Provides basic functions for writing to the screen and clearing it.
//

#include "vga.h"
#include "../lib/include/memset.h"

/**
 * @brief VGA text mode buffer address.
 *
 * The VGA text mode memory starts at physical address 0xB8000.
 * Each character cell is represented by a struct char_with_color.
 */
struct vga_char;
struct char_with_color *const VGA_ADDRESS = (void *) 0xB8000;

/**
 * @brief Current cursor position (line and column)
 */
static int line = 0;
static int pos = 0;

/**
 * @brief Default foreground and background colors
 */
static enum vga_colors fg = DEFAULT_FG_COLOR;
static enum vga_colors bg = DEFAULT_BG_COLOR;

void clear_vga()
{
    memset(VGA_ADDRESS, 0, VGA_HEIGHT * VGA_WIDTH);
    line = 0;
    pos = 0;
}

void write_buffer(struct char_with_color *tty_buffer)
{
    for (int i = 0; i < VGA_HEIGHT * VGA_WIDTH; i++)
    {
        VGA_ADDRESS[i] = tty_buffer[i];
    }
}
