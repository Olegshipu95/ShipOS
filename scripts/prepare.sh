#!/bin/sh
# Just generates output that is used in other tests
#
# Full log is ./report.log
# Tests log is ./tests.log
#

rm *.log
make ci
cat report.log | grep TEST > tests.log
