//
// Created by ShipOS developers on 10.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// Application Processor (AP) startup and management
//

#ifndef SHIP_OS_AP_STARTUP_H
#define SHIP_OS_AP_STARTUP_H

#include <inttypes.h>
#include <stdbool.h>

// Trampoline code location (must be below 1MB)
#define AP_TRAMPOLINE_ADDR 0x8000

// Stack size per AP (16KB)
#define AP_STACK_SIZE 0x4000

/**
 * @brief Initialize and start all Application Processors
 *
 * Copies trampoline code to low memory, sends INIT-SIPI-SIPI sequence
 * to wake up each AP, and waits for them to initialize.
 *
 * @param page_table Page table to use for APs
 * @return Number of APs successfully started
 */
uint32_t start_all_aps(void *page_table);

/**
 * @brief Entry point for Application Processors
 *
 * This function is called by each AP after it boots up.
 * It initializes the AP's Local APIC and other per-CPU structures.
 */
void ap_entry(void);

#endif // SHIP_OS_AP_STARTUP_H
