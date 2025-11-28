//
// Created by ShipOS developers on 28.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//

#include "../include/test.h"
#include "../include/logging.h"

int test_addition() {
    int a = 1;
    int b = 2;

    return a + b == 2;
}

void run_tests() {
    LOG("Test mode enabled, running tests");

    // TEST_REPORT("Failure", -1);
    // TEST_REPORT("Skip", 0);
    // TEST_REPORT("Success", 1);
    // TEST_REPORT_TIMED("TIMEOUT OK", 1, 5000, 10000);
    // TEST_REPORT_TIMED("TIMEOUT FAIL", 1, 5000, 1000);

    TEST_REPORT("Addition", CHECK(test_addition));

    LOG("No tests yet :(");
}
