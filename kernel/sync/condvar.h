//
// Created by ShipOS developers.
// Copyright (c) 2024-2026 SHIPOS. All rights reserved.
//

#ifndef UNTITLED_OS_CONDVAR_H
#define UNTITLED_OS_CONDVAR_H

#include "mutex.h"

/**
 * @brief Condition variable structure
 * Condition variables are used to wait for specific conditions 
 * while releasing an associated mutex.
 */
struct condvar {
    const char *name; // Name for debugging
};

void init_condvar(struct condvar *cv, const char *name);

/**
 * @brief Wait on a condition variable
 * Atomically releases the mutex and sleeps. When woken up,
 * the mutex is re-acquired before returning.
 * @param cv Pointer to the condition variable
 * @param m Pointer to the held mutex
 */
void cv_wait(struct condvar *cv, struct mutex *m);

/**
 * @brief Wake up threads waiting on the condition variable
 * In this implementation, both signal and broadcast wake up all threads.
 * The caller should use a 'while' loop to re-check the condition.
 */
void cv_signal(struct condvar *cv);

void cv_broadcast(struct condvar *cv);

#endif // UNTITLED_OS_CONDVAR_H