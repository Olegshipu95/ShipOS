#!/bin/sh
# Just generates output that is used in other tests
#
# Full log is ./report.log
# Tests log is ./tests.log
#

rm *.log
make ci | while IFS= read -r line; do
    echo $line | grep -E "^\[.+\].+$" | tr -d '\r' >> report.log
done
cat report.log | grep TEST > tests.log
