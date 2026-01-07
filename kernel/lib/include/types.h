//
// Created by ShipOS developers on 23.12.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>
#include <stddef.h>

#define NULL 0

/**
 * Gets the container structure from a pointer to its member.
 * @param ptr Pointer to the member.
 * @param type Type of the container structure.
 * @param member Name of the member.
 * @return Pointer to the container structure.
 **/
#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) - __builtin_offsetof(type, member)))

#endif // TYPES_H
