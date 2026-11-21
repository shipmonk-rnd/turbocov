#!/bin/sh
# Builds PHP from source into a prefix. The CI uses this script for the PHP builds that have no official image.
# Usage: dev/build-php.sh <version> <sha256 of the tar.xz> <prefix> [configure options]
# Environment: CC (default: cc)
set -eu

version="${1:?PHP version, for example 8.5.12}"
sha256="${2:?sha256 of php-<version>.tar.xz}"
prefix="${3:?install prefix}"
shift 3

source_dir="$(mktemp -d)"
cd "$source_dir"

curl -fsSL "https://www.php.net/distributions/php-$version.tar.xz" -o php.tar.xz
echo "$sha256  php.tar.xz" | sha256sum -c -
tar -xJf php.tar.xz --strip-components=1

./configure --prefix="$prefix" --with-config-file-path="$prefix/etc" \
    --disable-all --enable-cli --disable-cgi --disable-phpdbg --enable-opcache "$@" > configure.log 2>&1 \
    || { tail -50 configure.log; exit 1; }

make -j"$(getconf _NPROCESSORS_ONLN)" > make.log 2>&1 || { tail -80 make.log; exit 1; }
make install > install.log 2>&1
mkdir -p "$prefix/etc"

"$prefix/bin/php" -v
