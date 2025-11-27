//
// Created by ShipOS developers on 26.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//

#ifndef STR_UTILS_H
#define STR_UTILS_H

#include "x86_64.h"

/**
 * @brief Reverse a string in-place
 * @param str String to reverse
 * @param n Length of the string
 */
void reverse_str(char *str, int n);

/**
 * @brief Convert 64-bit integer to hexadecimal string
 * @param num 64-bit number
 * @param str Output buffer
 */
void ptoa(uint64_t num, char *str);

/**
 * @brief Convert integer to string with specified radix
 * @param num Integer value
 * @param str Output buffer
 * @param radix Base (e.g., 10, 16, 2)
 */
void itoa(int num, char *str, int radix);

#endif