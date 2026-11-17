#!/bin/sh
# Prints the versions of PHP, the coverage extensions and the test suite in the current directory.
set -eu

package() {
    php -r 'require "vendor/autoload.php"; echo Composer\InstalledVersions::getPrettyVersion($argv[1]);' "$1"
}

echo "PHP:               $(php -r 'echo PHP_VERSION;')"
echo "turbocov:          $(php -d extension=turbocov -r 'echo phpversion("turbocov");'), commit $TURBOCOV_COMMIT"
echo "PCOV:              $(php -d extension=pcov -r 'echo phpversion("pcov");')"
echo "Xdebug:            $(php -d zend_extension=xdebug -r 'echo phpversion("xdebug");')"
echo "Suite commit:      $(git rev-parse HEAD)"
echo "PHPUnit:           $(package phpunit/phpunit)"
echo "php-code-coverage: $(package phpunit/php-code-coverage)"
