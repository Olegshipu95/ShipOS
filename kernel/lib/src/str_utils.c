//
// Created by ShipOS developers on 26.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//

#include "../include/str_utils.h"

void reverse_str(char *str, int n) {
    int i = 0;
    int j = n - 1;
    while (i < j) {
        char tmp = str[i];
        str[i++] = str[j];
        str[j--] = tmp;
    }
}

int ptoa(uint64_t num, char *str) {
    int i = 0;

    do {
        // Check buffer for overflow with reserved byte for null-terminator
        if (i > MAX_DIGIT_BUFFER_SIZE - 1) {
            return -1;
        }
        // On each iteration convert integer value to corresponding
        // hex-representing char
        int rem = (num % 16);
        str[i++] = (rem > 9 ? 'a' - 10 : '0') + rem;
        num /= 16;
    } while (num);

    reverse_str(str, i);
    str[i] = 0;

    return 0;
}

int itoa(int num, char *str, int radix) {
    unsigned int n;
    int i = 0;
    int is_negative = 0;

    if (num < 0 && radix != 16) {
        is_negative = 1;
        n = (unsigned int)(-(long long)num);
    } else {
        n = (unsigned int)num;
    }

    do {
        // Check buffer for overflow with reserved byte for null-terminator
        if (i > MAX_DIGIT_BUFFER_SIZE - 1) {
            return -1;
        }
        // On each iteration convert integer value to corresponding
        // char for specified radix
        int rem = n % radix;
        str[i++] = (rem > 9 ? 'a' - 10 : '0') + rem;
        n /= radix;
    } while (n);

    if (is_negative) str[i++] = '-';
    reverse_str(str, i);
    str[i] = 0;

    return 0;
}
