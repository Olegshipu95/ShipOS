#!/bin/bash

echo "🔎 Scanning for critical issues..."

if grep -F "[PANIC]" report.log; then
    echo "🔥 PANIC detected in kernel log!"
    exit 1
fi

if grep -i "error" report.log; then
    echo "⚠️ Errors found in boot log:"
    grep -i "error" report.log
else
    echo "✨ No errors found"
fi
