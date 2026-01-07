//
// Created by ShipOS developers on 19.11.25.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#ifndef STRING_H
#define STRING_H

#include <inttypes.h>
#include <stddef.h>

// String length
size_t strlen(const char *str);

// String comparison
int strcmp(const char *s1, const char *s2);

// String copy
char *strcpy(char *dest, const char *src);

// String n-copy
char *strncpy(char *dest, const char *src, size_t n);

// Memory copy
void *memcpy(void *dest, const void *src, size_t n);

#endif // STRING_H
