//
// Created by ShipOS developers on 28.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//

#ifndef SHIPOS_LOGGING_H
#define SHIPOS_LOGGING_H

#include "../../tty/tty.h"
#include "../../serial/serial.h"

/**
 * @brief Utility macro for logging into TTY
 * 
 * @param format Format string
 * @param ... Variadic arguments
 */
#define LOG_TTY(format, ...) printf(format "\n", ##__VA_ARGS__)

/**
 * @brief Utility macro for logging into serial port
 * 
 * @param logger Name to be displayed in square brackets
 * @param format Format string
 * @param ... Variadic arguments
 */
#define LOG_SERIAL(logger, format, ...) serial_printf(get_default_serial_port(), "[" logger  "] " format "\n\r", ##__VA_ARGS__)


#ifdef DEBUG
/**
 * @brief Log function, always output to TTY and to serial if debug enabled
 * 
 * @param format Format string
 * @param ... Variadic arguments
 */
  #define LOG(format, ...) LOG_TTY(format, ##__VA_ARGS__); \
                           LOG_SERIAL("STDOUT", format, ##__VA_ARGS__)
#else
/**
 * @brief Log function, always output to TTY and to serial if debug enabled
 * 
 * @param format Format string
 * @param ... Variadic arguments
 */
  #define LOG(format, ...) LOG_TTY(format, ##__VA_ARGS__)
#endif


// Several presets for serial logging

#define DEBUG_SERIAL(format, ...) LOG_SERIAL("DEBUG", format, ##__VA_ARGS__)
#define PANIC_SERIAL(format, ...) LOG_SERIAL("PANIC", format, ##__VA_ARGS__)


/**
 * @brief Utility function to report test results
 * 
 * @param name Name of testcase
 * @param status Test result to report (-1 = failure, 0 = skipped, 1 = passed)
 */
#define TEST_REPORT(name, status) \
  if (status > 0) \
    LOG_SERIAL("TEST", name " - OK"); \
  else if (status < 0) \
    LOG_SERIAL("TEST", name " - Failure"); \
  else \
    LOG_SERIAL("TEST", name " - Skipped")

/**
 * @brief Utility function to report test results with time limit
 * 
 * @param name Name of testcase
 * @param status Test result to report (-1 = failure, 0 = skipped, 1 = passed)
 * @param runtime_ms Test runtime in ms
 * @param limit_ms Max test runtime in ms
 */
#define TEST_REPORT_TIMED(name, status, runtime_ms, limit_ms) \
if (runtime_ms >= limit_ms) \
  LOG_SERIAL("TEST", name " - Timeout (%dms > %dms)", runtime_ms, limit_ms); \
else \
  TEST_REPORT(name, status)

#endif