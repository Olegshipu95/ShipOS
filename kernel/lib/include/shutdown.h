//
// Created by untitled-os developers on 28.11.25.
// Copyright (c) 2025 untitled-os. All rights reserved.
//
// System shutdown interface for untitled-os kernel.
// Provides graceful shutdown via ACPI or port I/O methods.
//

#ifndef untitled-os_SHUTDOWN_H
#define untitled-os_SHUTDOWN_H

/**
 * @brief Gracefully shut down the operating system
 * 
 * Attempts to power off the system using multiple methods:
 * 1. QEMU/Bochs shutdown port (0x604)
 * 2. Legacy APM interface
 * 3. ACPI sleep state (if available)
 * 
 * If hardware shutdown is not supported, the system will halt.
 * This function does not return.
 */
void shutdown(void);

#endif // untitled-os_SHUTDOWN_H
