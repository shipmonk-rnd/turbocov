#!/bin/sh
# Runs each given PHP file with PCOV and with turbocov and checks the line data of turbocov against PCOV.
# Usage: dev/compare-with-pcov.sh <php-version> <file>...
#
# Result for each file:
#   same       the same executable lines and the same executed lines
#   stricter   the same executable lines; PCOV reports more executed lines (operands that PHP skips in a multi-line expression)
#   DIFFERENT  the executable lines differ, or turbocov reports an executed line that PCOV does not report (exit status 1)
set -eu

version="${1:?PHP version, for example 8.5}"
shift
root="$(cd "$(dirname "$0")/.." && pwd)"
image="turbocov-dev:$version"

docker image inspect "$image" > /dev/null 2>&1 || docker build -q --build-arg "PHP_VERSION=$version" -t "$image" "$root/dev" > /dev/null
[ -f "$root/build/php-$version/turbocov/turbocov.so" ] || "$root/dev/build.sh" "$version" > /dev/null

status=0

for file in "$@"; do
    dir="$(cd "$(dirname "$file")" && pwd)"
    name="$(basename "$file")"
    container="docker run --rm -i --memory 2g --memory-swap 2g -v $root:/src:ro -v $dir:/code:ro $image"
    run="$container php -d opcache.enable_cli=0"
    expected="$($run -d extension=pcov -d pcov.directory=/code /src/dev/collect.php pcov "/code/$name")"
    actual="$($run -d "extension=/src/build/php-$version/turbocov/turbocov.so" -d turbocov.collect_paths=/code /src/dev/collect.php turbocov "/code/$name")"

    printf '%s\n%s\n' "$expected" "$actual" | $container php /src/dev/compare.php "$file" || status=1
done

exit $status
