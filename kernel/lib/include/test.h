//
// Created by ShipOS developers on 28.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//

/**
 * @brief Maps test result to 1 for success and -1 for failure
 * 
 * @param result Test result, 1 for success, 0 for failure
 * 
 */
#define CHECK(result) (result ? 1 : -1)

#ifndef SHIPOS_TEST_H
#define SHIPOS_TEST_H

/**
 * @brief Run all testcases
 * 
 */
void run_tests();

#endif
