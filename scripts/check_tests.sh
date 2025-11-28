#!/bin/sh

set -e

chmod +x ./scripts/tests/*.sh 2>/dev/null || true

for script in ./scripts/tests/*.sh; do
    # Skip if no scripts found
    [ -e "$script" ] || continue

    bash "$script"
done
