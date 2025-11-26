//
// Created by ShipOS developers
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#ifndef PIT_H
#define PIT_H

/**
 * @brief Initialize the Programmable Interval Timer (PIT)
 *
 * Configures PIT channel 0 in mode 3 (square wave generator) to generate
 * timer interrupts. This must be called before using send_values_to_sched().
 */
extern void init_pit();

/**
 * @brief Send a 16-bit reload value to PIT channel 0
 *
 * The value determines the interval at which timer interrupts occur.
 * Low byte is sent first, then high byte. Example value: 0x7FFF → ~36 Hz.
 */
extern void send_values_to_sched();

/**
 * @brief Stop the PIT timer
 *
 * Sends a zero value to PIT channel 0, effectively stopping timer interrupts.
 */
extern void stop_timer();

#endif // PIT_H
