#!/bin/sh
# Runs the tests of php-src without turbocov and with turbocov, and fails when the result of a test differs.
# turbocov changes the opcode handlers of each file that PHP compiles. These tests show when that changes what PHP does.
# The PHP in PATH runs the tests. The CI uses this script.
# Usage: dev/php-src-tests.sh <version> <sha256 of the tar.xz> [test directory in php-src...]
# Environment:
#   TURBOCOV_SO   the extension file (default: modules/turbocov.so)
#   JOBS          the number of test workers (default: the number of processors)
set -eu

version="${1:?PHP version, for example 8.5.12}"
sha256="${2:?sha256 of php-<version>.tar.xz}"
shift 2
[ $# -gt 0 ] || set -- Zend/tests tests ext/opcache/tests ext/reflection/tests ext/spl/tests ext/standard/tests

root="$(cd "$(dirname "$0")/.." && pwd)"
extension="$(cd "$(dirname "${TURBOCOV_SO:-$root/modules/turbocov.so}")" && pwd)/$(basename "${TURBOCOV_SO:-turbocov.so}")"
jobs="${JOBS:-$(getconf _NPROCESSORS_ONLN)}"
work="$(mktemp -d)"
tree="$work/php-src"
log="$work/collection.log"

[ -f "$extension" ] || { echo "$extension does not exist. Build turbocov first." >&2; exit 1; }

curl -fsSL "https://www.php.net/distributions/php-$version.tar.xz" -o "$work/php.tar.xz"
echo "$sha256  $work/php.tar.xz" | sha256sum -c - > /dev/null

# Each run starts from the same files. A test of the first run can leave files that change a test of the second run.
unpack() {
    rm -rf "$tree"
    mkdir "$tree"
    members="php-$version/run-tests.php"
    for directory in "$@"; do members="$members php-$version/$directory"; done
    # shellcheck disable=SC2086
    tar -xJf "$work/php.tar.xz" -C "$tree" --strip-components=1 $members
}

# run <name> <"" or "turbocov"> <test or directory...>: writes $work/<name>.txt with a line "<result><tab><test>" for each test.
run() {
    name="$1"
    mode="$2"
    shift 2
    options="-d auto_prepend_file=$root/dev/php-src-tests-prepend.php"

    if [ "$mode" = turbocov ]; then
        options="$options -d extension=$extension -d turbocov.collect_paths=$tree"
    fi

    # The exit status is not 0 when a test fails. Some tests fail in both runs, for example the tests that need a network.
    # shellcheck disable=SC2086
    (cd "$tree" && NO_INTERACTION=1 SKIP_SLOW_TESTS=1 TEST_PHP_EXECUTABLE="$(command -v php)" TURBOCOV_PHP_SRC_TESTS_LOG="$log" \
        php run-tests.php -q -j"$jobs" $options -W "$work/$name.unsorted" "$@" > "$work/$name.log" 2>&1) || true

    [ -s "$work/$name.unsorted" ] || { tail -30 "$work/$name.log"; echo "The run \"$name\" gave no results." >&2; exit 1; }
    sed "s|	$tree/|	|" "$work/$name.unsorted" | sort -t "$(printf '\t')" -k 2 > "$work/$name.txt"
    echo "$name: $(cut -f 1 "$work/$name.txt" | sort | uniq -c | tr -s ' \n' ' ')"
}

# differences <name of the run without turbocov> <name of the run with turbocov>: prints "<test><tab><result> -> <result>".
differences() {
    awk -F '\t' 'NR == FNR { without[$2] = $1; next } without[$2] != $1 { print $2 "\t" without[$2] " -> " $1 }' "$work/$1.txt" "$work/$2.txt"
}

php -v | head -1

unpack "$@"
run without "" "$@"
unpack "$@"
run with turbocov "$@"

collected="$(grep -c '^collected$' "$log" || true)"
passed="$(grep -c '^PASSED' "$work/with.txt" || true)"
echo "turbocov collected in $collected PHP processes."

status=0

# A defect that turns the collection off in most tests would make this check pass with no meaning.
if [ "$collected" -lt "$((passed / 2))" ]; then
    echo "turbocov must collect in most of the $passed tests that pass." >&2
    status=1
fi

if grep -q '^PROBLEM' "$log"; then
    grep '^PROBLEM' "$log" | sort | uniq -c | head -50 >&2
    status=1
fi

differences without with > "$work/differences.txt"

# Some tests of php-src do not give the same result each time. Only a difference that occurs two times counts.
if [ -s "$work/differences.txt" ]; then
    echo "$(wc -l < "$work/differences.txt") tests differ. Second run of these tests:"
    tests="$(cut -f 1 "$work/differences.txt" | tr '\n' ' ')"
    # shellcheck disable=SC2086
    run without-again "" $tests
    # shellcheck disable=SC2086
    run with-again turbocov $tests
    differences without-again with-again > "$work/differences.txt"
fi

if [ -s "$work/differences.txt" ]; then
    echo "turbocov changes the result of these tests:" >&2
    cat "$work/differences.txt" >&2

    cut -f 1 "$work/differences.txt" | head -20 | while read -r test; do
        echo "--- ${test%.phpt}.diff" >&2
        head -60 "$tree/${test%.phpt}.diff" >&2 || true
    done

    status=1
fi

[ "$status" = 0 ] && echo "turbocov does not change the result of a test."
rm -rf "$work"
exit "$status"
