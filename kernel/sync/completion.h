//
// Created by ShipOS developers.
// Copyright (c) 2024-2026 SHIPOS. All rights reserved.
//

#ifndef UNTITLED_OS_COMPLETION_H
#define UNTITLED_OS_COMPLETION_H

#include "spinlock.h"

struct completion {
    uint32_t done;
    struct spinlock lock;
};

void init_completion(struct completion *c, const char *name);
void wait_for_completion(struct completion *c);
void complete(struct completion *c);
void complete_all(struct completion *c);

#endif // UNTITLED_OS_COMPLETION_H