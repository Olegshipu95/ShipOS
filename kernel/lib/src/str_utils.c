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

void ptoa(uint64_t num, char *str) {
    int i = 0;

    do {
        int rem = (num % 16);
        str[i++] = (rem > 9 ? 'a' - 10 : '0') + rem;
        num /= 16;
    } while (num);

    reverse_str(str, i);
    str[i] = 0;
}

void itoa(int num, char *str, int radix) {
    int i = 0;
    int is_negative = 0;
    if (num < 0 && radix != 16) {
        is_negative = 1;
        num *= -1;
    }

    do {
        int rem = (num % radix);
        str[i++] = (rem > 9 ? 'a' - 10 : '0') + rem;
        num /= radix;
    } while (num);

    if (is_negative) str[i++] = '-';
    reverse_str(str, i);
    str[i] = 0;
}
