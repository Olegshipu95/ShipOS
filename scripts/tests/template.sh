#!/bin/bash

check_success() {
    if grep -q "$2 - Passed" tests.log; then
        echo "✅ '$2' - OK"
    else
        echo "❌ '$2' - Failed"
    fi
}

check_success "Addition"
