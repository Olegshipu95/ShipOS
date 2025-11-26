//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#include "../include/memset.h"
// #include "../include/stdint.h"
#include <inttypes.h>

void *
memset(void *ptr, int value, size_t num)
{
    unsigned char *byte_ptr = (unsigned char *) ptr;
    unsigned char byte_value = (unsigned char) value;

    for (size_t i = 0; i < num; i++)
    {
        *byte_ptr++ = byte_value;
    }

    return ptr;
}
