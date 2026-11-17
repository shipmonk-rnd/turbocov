#!/bin/sh
# Builds turbocov in this directory with the PHP that is in PATH, and runs the tests. The CI uses this script.
# Usage: dev/build-in-place.sh [turbocov|pcov]
# Environment:
#   CFLAGS, LDFLAGS   added to the build
#   RUN_TESTS_FLAGS   added to the test run, for example --asan
#   EXPECT_SKIPS      the number of tests that the run must skip (default: 0)
set -eu

name="${1:-turbocov}"
flags="--enable-turbocov"
tests="tests/*.phpt"

if [ "$name" = "pcov" ]; then
    flags="$flags --enable-turbocov-impersonate-pcov"
    tests="tests/impersonate-pcov"
fi

uname -sm
php -v
php -n -r 'echo "VM kind: ", defined("ZEND_VM_KIND") ? ZEND_VM_KIND : "not reported by this PHP version", "\n";'

phpize > /dev/null
./configure $flags > /dev/null
make -j"$(getconf _NPROCESSORS_ONLN)" CFLAGS="-O2 -g -Wall -Wextra -Wno-unused-parameter -Werror ${CFLAGS:-}"

registered_name="turbocov"
[ "$name" = "pcov" ] && registered_name="pcov"

# The test runner skips each test when PHP cannot load the extension. Then no test fails, and no test runs.
php -n -d extension_dir=modules -d extension=turbocov.so -r "exit(extension_loaded('$registered_name') ? 0 : 1);" || {
    echo "PHP cannot load modules/turbocov.so." >&2
    exit 1
}

extension_dir="$(php-config --extension-dir)"

# "make test" removes zend_extension lines from its ini file. A shared OPcache must come in again.
opcache=""
[ -f "$extension_dir/opcache.so" ] && opcache="-d zend_extension=$extension_dir/opcache.so"

# "make test" uses modules/ as the extension directory. Two tests load PCOV and Xdebug from there.
for extension in pcov xdebug; do
    [ -f "$extension_dir/$extension.so" ] && ln -sf "$extension_dir/$extension.so" "modules/$extension.so"
done

# A pipe to "tee" hides the exit status of "make test", and sh has no pipefail option.
log="$(mktemp)"
status=0
NO_INTERACTION=1 REPORT_EXIT_STATUS=1 make test TESTS="${RUN_TESTS_FLAGS:-} $opcache --show-diff $tests" > "$log" 2>&1 || status=$?
cat "$log"

if [ "$status" != 0 ]; then
    echo "The tests failed." >&2
    exit "$status"
fi

if ! grep -q "Tests skipped *: *${EXPECT_SKIPS:-0} " "$log"; then
    echo "The run must skip ${EXPECT_SKIPS:-0} tests. Set EXPECT_SKIPS when a skip is correct." >&2
    exit 1
fi
