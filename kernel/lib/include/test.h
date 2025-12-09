//
// Created by ShipOS developers on 28.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//

/**
 * @brief Maps test result to 1 for success and -1 for failure
 * 
 * @param test_fn Test function without params, should return 1 for pass, 0 for failure
 * 
 */
#define CHECK(test_fn) (test_fn() ? 1 : -1)

#ifndef SHIPOS_TEST_H
#define SHIPOS_TEST_H

/**
 * @brief Run all testcases
 * 
 */
void run_tests();

#endif
