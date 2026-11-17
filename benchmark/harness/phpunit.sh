#!/bin/sh
# Runs the test suite in the current directory with one coverage extension loaded.
# Usage: phpunit.sh <none|turbocov|pcov|xdebug> <source directory>
set -eu

mode="$1"
source="$(pwd)/$2"

case "$mode" in
    none) set -- ;;
    turbocov) set -- -d extension=turbocov -d "turbocov.collect_paths=$source" ;;
    pcov) set -- -d extension=pcov -d "pcov.directory=$source" ;;
    xdebug) set -- -d zend_extension=xdebug -d xdebug.mode=coverage ;;
    *) echo "Unknown mode: $mode" >&2; exit 1 ;;
esac

config=phpunit.xml
[ -f "$config" ] || config=phpunit.xml.dist

php /harness/add-extension.php "$config" phpunit.benchmark.xml

# --no-coverage: a configuration file with a coverage report must not start the coverage library of PHPUnit.
TURBOCOV_BENCHMARK_MODE="$mode" TURBOCOV_BENCHMARK_SOURCE="$source" exec php \
    -d opcache.enable_cli=0 \
    -d memory_limit=-1 \
    -d auto_prepend_file=/harness/autoload.php \
    "$@" \
    vendor/bin/phpunit -c phpunit.benchmark.xml --no-coverage --colors=never
