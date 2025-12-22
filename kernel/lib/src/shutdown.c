//
// Created by untitled-os developers on 28.11.25.
// Copyright (c) 2025 untitled-os. All rights reserved.
//
// System shutdown implementation for untitled-os kernel.
//

#include "../include/shutdown.h"
#include "../include/logging.h"
#include "../include/x86_64.h"

/**
 * @brief QEMU debug-exit device port
 * Using isa-debug-exit device at port 0xf4
 * Exit code = (value << 1) | 1, so 0x00 gives exit code 1
 */
#define QEMU_DEBUG_EXIT_PORT 0xf4
#define QEMU_EXIT_SUCCESS 0x10  // Will result in QEMU exit code 33 ((0x10 << 1) | 1)

/**
 * @brief ACPI shutdown port for QEMU/Bochs
 * Writing 0x2000 to port 0x604 triggers a shutdown in QEMU
 */
#define QEMU_SHUTDOWN_PORT 0x604
#define QEMU_SHUTDOWN_VALUE 0x2000

/**
 * @brief Legacy shutdown port for QEMU/Bochs
 * Writing 0x2000 to port 0x604 triggers a shutdown in QEMU
 */
#define LEGACY_QEMU_SHUTDOWN_PORT 0x501
#define LEGACY_QEMU_SHUTDOWN_VALUE 0x01

/**
 * @brief Legacy APM (Advanced Power Management) interface
 * Used by older systems for power management
 */
#define APM_PORT 0xB004
#define APM_SHUTDOWN_VALUE 0x2000

void shutdown(void) {
    LOG("System shutdown initiated");
    LOG_SERIAL("SHUTDOWN", "Shutting down untitled-os...");

    // Method 1: QEMU debug-exit device (most reliable for automated testing)
    LOG_SERIAL("SHUTDOWN", "Attempting QEMU debug-exit (port 0xf4)");
    outl(QEMU_DEBUG_EXIT_PORT, QEMU_EXIT_SUCCESS);

    // Method 2: QEMU/Bochs shutdown via port 0x604
    LOG_SERIAL("SHUTDOWN", "Attempting QEMU/Bochs shutdown (port 0x604)");
    outw(QEMU_SHUTDOWN_PORT, QEMU_SHUTDOWN_VALUE);

    // Legacy APM shutdown
    LOG_SERIAL("SHUTDOWN", "Attempting APM shutdown (port 0xB004)");
    outw(APM_PORT, APM_SHUTDOWN_VALUE);

    // Try older QEMU shutdown port
    LOG_SERIAL("SHUTDOWN", "Attempting legacy QEMU shutdown (port 0x501)");
    outb(LEGACY_QEMU_SHUTDOWN_PORT, LEGACY_QEMU_SHUTDOWN_VALUE);

    // Hardware shutdown failed
    LOG_SERIAL("SHUTDOWN", "Hardware shutdown not supported");
    LOG("Hardware shutdown not supported - system halted");
    print("System shutdown requested but hardware shutdown is not available.\n");
    print("It is now safe to power off your computer.\n");

    // Halt the CPU indefinitely
    while (1) {
        asm volatile("hlt");
    }
}
