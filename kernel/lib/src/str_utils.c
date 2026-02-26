//
// Created by ShipOS developers on 26.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//
// String utility functions for kernel formatting and conversion.
//

#include "../include/str_utils.h"
#include "../include/memset.h"

// ============================================================================
// Basic String Functions
// ============================================================================

void reverse_str(char *str, int n) {
    int i = 0;
    int j = n - 1;
    while (i < j) {
        char tmp = str[i];
        str[i++] = str[j];
        str[j--] = tmp;
    }
}

size_t strlen(const char *str) {
    if (str == NULL) return 0;
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

// ============================================================================
// Number to String Conversion
// ============================================================================

static const char digits_lower[] = "0123456789abcdef";
static const char digits_upper[] = "0123456789ABCDEF";

int utoa64(uint64_t num, char *str, int radix, int uppercase) {
    if (radix < 2 || radix > 16) radix = 10;
    
    const char *digits = uppercase ? digits_upper : digits_lower;
    int i = 0;
    
    if (num == 0) {
        str[i++] = '0';
    } else {
        while (num) {
            str[i++] = digits[num % radix];
            num /= radix;
        }
    }
    
    reverse_str(str, i);
    str[i] = '\0';
    return i;
}

int itoa64(int64_t num, char *str, int radix) {
    if (radix < 2 || radix > 16) radix = 10;
    
    int i = 0;
    int is_negative = 0;
    uint64_t n;
    
    if (num < 0 && radix == 10) {
        is_negative = 1;
        n = (uint64_t)(-(num + 1)) + 1;  // Handle INT64_MIN correctly
    } else {
        n = (uint64_t)num;
    }
    
    if (n == 0) {
        str[i++] = '0';
    } else {
        while (n) {
            str[i++] = digits_lower[n % radix];
            n /= radix;
        }
    }
    
    if (is_negative) str[i++] = '-';
    reverse_str(str, i);
    str[i] = '\0';
    return i;
}

// Legacy functions for backward compatibility
int ptoa(uint64_t num, char *str) {
    utoa64(num, str, 16, 0);
    return 0;
}

int itoa(int num, char *str, int radix) {
    if (radix == 10) {
        itoa64((int64_t)num, str, radix);
    } else {
        utoa64((uint64_t)(unsigned int)num, str, radix, 0);
    }
    return 0;
}

// ============================================================================
// Formatted Number Output
// ============================================================================

int format_unsigned(uint64_t num, char *buf, int radix, struct fmt_spec *spec) {
    char numbuf[MAX_DIGIT_BUFFER_SIZE];
    int uppercase = (spec->flags & FMT_FLAG_UPPER) != 0;
    int numlen = utoa64(num, numbuf, radix, uppercase);
    
    // Determine prefix
    const char *prefix = "";
    int prefix_len = 0;
    if (spec->flags & FMT_FLAG_HASH) {
        if (radix == 16 && num != 0) {
            prefix = uppercase ? "0X" : "0x";
            prefix_len = 2;
        } else if (radix == 8 && num != 0 && numbuf[0] != '0') {
            prefix = "0";
            prefix_len = 1;
        } else if (radix == 2 && num != 0) {
            prefix = uppercase ? "0B" : "0b";
            prefix_len = 2;
        }
    }
    
    // Calculate padding
    int precision = spec->precision;
    if (precision < 0) precision = 1;  // Default precision is 1
    
    int num_zeros = 0;
    if (numlen < precision) {
        num_zeros = precision - numlen;
    }
    
    int total_digits = numlen + num_zeros;
    int total_len = prefix_len + total_digits;
    int padding = 0;
    if (spec->width > total_len) {
        padding = spec->width - total_len;
    }
    
    // Zero-padding goes after prefix but before digits
    int zero_pad = 0;
    if ((spec->flags & FMT_FLAG_ZERO) && !(spec->flags & FMT_FLAG_LEFT) && spec->precision < 0) {
        zero_pad = padding;
        padding = 0;
    }
    
    // Build output
    int pos = 0;
    
    // Left padding (spaces)
    if (!(spec->flags & FMT_FLAG_LEFT)) {
        for (int i = 0; i < padding; i++) buf[pos++] = ' ';
    }
    
    // Prefix
    for (int i = 0; i < prefix_len; i++) buf[pos++] = prefix[i];
    
    // Zero padding
    for (int i = 0; i < zero_pad; i++) buf[pos++] = '0';
    
    // Precision zeros
    for (int i = 0; i < num_zeros; i++) buf[pos++] = '0';
    
    // Number
    for (int i = 0; i < numlen; i++) buf[pos++] = numbuf[i];
    
    // Right padding (spaces)
    if (spec->flags & FMT_FLAG_LEFT) {
        for (int i = 0; i < padding; i++) buf[pos++] = ' ';
    }
    
    buf[pos] = '\0';
    return pos;
}

int format_signed(int64_t num, char *buf, int radix, struct fmt_spec *spec) {
    char numbuf[MAX_DIGIT_BUFFER_SIZE];
    
    // Determine sign
    char sign = 0;
    uint64_t abs_num;
    if (num < 0) {
        sign = '-';
        abs_num = (uint64_t)(-(num + 1)) + 1;  // Handle INT64_MIN
    } else {
        abs_num = (uint64_t)num;
        if (spec->flags & FMT_FLAG_PLUS) {
            sign = '+';
        } else if (spec->flags & FMT_FLAG_SPACE) {
            sign = ' ';
        }
    }
    
    int numlen = utoa64(abs_num, numbuf, radix, 0);
    
    // Calculate padding
    int precision = spec->precision;
    if (precision < 0) precision = 1;
    
    int num_zeros = 0;
    if (numlen < precision) {
        num_zeros = precision - numlen;
    }
    
    int sign_len = sign ? 1 : 0;
    int total_digits = numlen + num_zeros;
    int total_len = sign_len + total_digits;
    int padding = 0;
    if (spec->width > total_len) {
        padding = spec->width - total_len;
    }
    
    // Zero-padding
    int zero_pad = 0;
    if ((spec->flags & FMT_FLAG_ZERO) && !(spec->flags & FMT_FLAG_LEFT) && spec->precision < 0) {
        zero_pad = padding;
        padding = 0;
    }
    
    // Build output
    int pos = 0;
    
    // Left padding (spaces)
    if (!(spec->flags & FMT_FLAG_LEFT)) {
        for (int i = 0; i < padding; i++) buf[pos++] = ' ';
    }
    
    // Sign
    if (sign) buf[pos++] = sign;
    
    // Zero padding
    for (int i = 0; i < zero_pad; i++) buf[pos++] = '0';
    
    // Precision zeros
    for (int i = 0; i < num_zeros; i++) buf[pos++] = '0';
    
    // Number
    for (int i = 0; i < numlen; i++) buf[pos++] = numbuf[i];
    
    // Right padding (spaces)
    if (spec->flags & FMT_FLAG_LEFT) {
        for (int i = 0; i < padding; i++) buf[pos++] = ' ';
    }
    
    buf[pos] = '\0';
    return pos;
}

int format_string(const char *str, char *buf, size_t buf_size, struct fmt_spec *spec) {
    if (str == NULL) str = "(null)";
    
    size_t slen = strlen(str);
    
    // Apply precision (maximum chars to output)
    if (spec->precision >= 0 && (size_t)spec->precision < slen) {
        slen = spec->precision;
    }
    
    int padding = 0;
    if (spec->width > 0 && (size_t)spec->width > slen) {
        padding = spec->width - slen;
    }
    
    size_t pos = 0;
    
    // Left padding
    if (!(spec->flags & FMT_FLAG_LEFT)) {
        for (int i = 0; i < padding && pos < buf_size - 1; i++) {
            buf[pos++] = ' ';
        }
    }
    
    // String content
    for (size_t i = 0; i < slen && pos < buf_size - 1; i++) {
        buf[pos++] = str[i];
    }
    
    // Right padding
    if (spec->flags & FMT_FLAG_LEFT) {
        for (int i = 0; i < padding && pos < buf_size - 1; i++) {
            buf[pos++] = ' ';
        }
    }
    
    buf[pos] = '\0';
    return pos;
}
