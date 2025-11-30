#!/bin/bash
# Just sample test to check addittion

check() {
    if grep -q "$1 - Skipped" tests.log; then
        echo "$1 - Skipped"
    else 
        if grep -q "$1 - Passed" tests.log; then
            echo "✅ $1 - OK"
        else 
            echo "❌ $1 - Failed"
        fi
    fi
}

check "Addition"
