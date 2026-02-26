#!/bin/bash

set -e

chmod +x ./scripts/tests/*.sh 2>/dev/null || true

passed=0
failed=0

for script in ./scripts/tests/*.sh; do
    # Skip if no scripts found
    [ -e "$script" ] || continue

    if bash "$script"; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
    fi
done

let total=$passed+$failed
echo "📝 Test sets passed: $passed / $total"

if [ $failed -gt 0 ]; then
    exit 1
fi
