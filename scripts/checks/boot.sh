#!/bin/sh

echo "🔍 Checking for expected boot messages..."

check() {
    if grep -q "$2" report.log; then
        echo "✅ $1"
    else
        echo "❌ $1 missing!"
        exit 1
    fi
}

check "Serial port initialized" "\[SERIAL\] Serial ports initialized successfully"
check "TTY subsystem initialized" "\[BOOT\] TTY subsystem initialized"
check "Memory subsystem initialized" "\[MEMORY\] Physical memory initialized"
check "Kernel boot completed" "\[KERNEL\] Boot sequence completed successfully"

echo "🎉 All boot checks passed!"