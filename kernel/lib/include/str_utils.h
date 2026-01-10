//
// Created by ShipOS developers on 26.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//
// String utility functions for kernel formatting and conversion.
//

#ifndef STR_UTILS_H
#define STR_UTILS_H

#define MAX_DIGIT_BUFFER_SIZE 68  // Enough for 64-bit binary + sign + null

#include "x86_64.h"
#include <stddef.h>

// ============================================================================
// Printf Format Flags
// ============================================================================

#define FMT_FLAG_LEFT    (1 << 0)  // '-' left justify
#define FMT_FLAG_PLUS    (1 << 1)  // '+' always show sign
#define FMT_FLAG_SPACE   (1 << 2)  // ' ' space before positive numbers
#define FMT_FLAG_HASH    (1 << 3)  // '#' alternate form (0x, 0, etc.)
#define FMT_FLAG_ZERO    (1 << 4)  // '0' zero-pad
#define FMT_FLAG_UPPER   (1 << 5)  // uppercase hex (X vs x)

// Length modifiers
#define FMT_LEN_DEFAULT  0
#define FMT_LEN_HH       1  // char
#define FMT_LEN_H        2  // short
#define FMT_LEN_L        3  // long
#define FMT_LEN_LL       4  // long long
#define FMT_LEN_Z        5  // size_t

/**
 * @brief Format specification parsed from printf format string
 */
struct fmt_spec {
    int flags;       // FMT_FLAG_* combinations
    int width;       // minimum field width (-1 if not specified)
    int precision;   // precision (-1 if not specified)
    int length;      // FMT_LEN_* value
};

// ============================================================================
// Basic String Functions
// ============================================================================

/**
 * @brief Reverse a string in-place
 * @param str String to reverse
 * @param n Length of the string
 */
void reverse_str(char *str, int n);

/**
 * @brief Get string length
 * @param str Null-terminated string
 * @return Length of string (not including null terminator)
 */
size_t strlen(const char *str);

/**
 * @brief Copy string
 * @param dest Destination buffer
 * @param src Source string
 * @return Pointer to dest
 */
char *strcpy(char *dest, const char *src);

// ============================================================================
// Number to String Conversion
// ============================================================================

/**
 * @brief Convert unsigned 64-bit integer to string
 * @param num Number to convert
 * @param str Output buffer (must be at least MAX_DIGIT_BUFFER_SIZE)
 * @param radix Base (2-36)
 * @param uppercase Use uppercase letters for hex
 * @return Number of characters written (not including null)
 */
int utoa64(uint64_t num, char *str, int radix, int uppercase);

/**
 * @brief Convert signed 64-bit integer to string
 * @param num Number to convert
 * @param str Output buffer (must be at least MAX_DIGIT_BUFFER_SIZE)
 * @param radix Base (2-36)
 * @return Number of characters written (not including null)
 */
int itoa64(int64_t num, char *str, int radix);

/**
 * @brief Convert 64-bit integer to hexadecimal string (legacy)
 * @param num 64-bit number
 * @param str Output buffer
 * @return 0 on success, -1 on error
 */
int ptoa(uint64_t num, char *str);

/**
 * @brief Convert integer to string with specified radix (legacy)
 * @param num Integer value
 * @param str Output buffer
 * @param radix Base (e.g., 10, 16, 2)
 * @return 0 on success, -1 on error
 */
int itoa(int num, char *str, int radix);

// ============================================================================
// Formatted Number Output
// ============================================================================

/**
 * @brief Format an unsigned integer with full printf-style formatting
 * @param num Number to format
 * @param buf Output buffer (must be large enough)
 * @param radix Base (2, 8, 10, 16)
 * @param spec Format specification
 * @return Number of characters written
 */
int format_unsigned(uint64_t num, char *buf, int radix, struct fmt_spec *spec);

/**
 * @brief Format a signed integer with full printf-style formatting
 * @param num Number to format
 * @param buf Output buffer (must be large enough)
 * @param radix Base (usually 10)
 * @param spec Format specification
 * @return Number of characters written
 */
int format_signed(int64_t num, char *buf, int radix, struct fmt_spec *spec);

/**
 * @brief Format a string with width and precision
 * @param str String to format (can be NULL)
 * @param buf Output buffer
 * @param buf_size Size of output buffer
 * @param spec Format specification
 * @return Number of characters written
 */
int format_string(const char *str, char *buf, size_t buf_size, struct fmt_spec *spec);

#endif