#!/bin/sh
# Builds turbocov in this directory with the PHP that is in PATH, and runs the tests. The CI uses this script.
# Usage: dev/build-in-place.sh [turbocov|pcov]
# Environment:
#   CFLAGS, LDFLAGS   added to the build
#   RUN_TESTS_FLAGS   added to the test run, for example --asan
#   EXPECT_NO_SKIPS   1 = a skipped test is an error
set -eu

name="${1:-turbocov}"
flags="--enable-turbocov"
tests="tests/*.phpt"

if [ "$name" = "pcov" ]; then
    flags="$flags --enable-turbocov-impersonate-pcov"
    tests="tests/impersonate-pcov"
fi

phpize > /dev/null
./configure $flags > /dev/null
make -j"$(nproc)" CFLAGS="-O2 -g -Wall -Wextra -Wno-unused-parameter -Werror ${CFLAGS:-}"

# "make test" removes zend_extension lines from its ini file. A shared OPcache must come in again.
opcache="$(php-config --extension-dir)/opcache.so"
[ -f "$opcache" ] && opcache="-d zend_extension=$opcache" || opcache=""

log="$(mktemp)"
NO_INTERACTION=1 REPORT_EXIT_STATUS=1 make test TESTS="${RUN_TESTS_FLAGS:-} $opcache --show-diff $tests" | tee "$log"

if [ "${EXPECT_NO_SKIPS:-0}" = 1 ] && ! grep -q 'Tests skipped *: *0 ' "$log"; then
    echo "A test was skipped, and EXPECT_NO_SKIPS is set." >&2
    exit 1
fi
