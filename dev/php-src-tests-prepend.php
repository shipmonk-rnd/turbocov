<?php

// dev/php-src-tests.sh loads this file before each test of php-src, with turbocov and without it.
// The two runs must do the same work here. So the file creates no object and defines the same function in both runs:
// an object more moves the object numbers that var_dump() prints in the tests.

function __turbocov_php_src_tests_end(): void
{
    if (!function_exists('turbocov\stop')) {
        return;
    }

    $log = getenv('TURBOCOV_PHP_SRC_TESTS_LOG');
    $executed = \turbocov\stop();

    // A test can start a PHP process with an empty environment. Some tests disable functions, so this file calls only a few.
    if ($log === false || $log === '') {
        return;
    }

    // "@": a test with open_basedir or chroot() cannot write the log, and a warning changes its output.
    if ($executed !== []) {
        @file_put_contents($log, 'collected ' . ($_SERVER['SCRIPT_FILENAME'] ?? '') . "\n", FILE_APPEND);
    }

    foreach ($executed as $file => $lines) {
        $executable = \turbocov\get_executable_lines([$file])[$file] ?? [];
        $unknown = array_keys(array_diff_key($lines, $executable));

        if ($unknown !== []) {
            @file_put_contents($log, "PROBLEM $file: executed lines that are not executable: " . implode(', ', $unknown) . "\n", FILE_APPEND);
        }
    }
}

if (function_exists('turbocov\start')) {
    try {
        \turbocov\start();
        register_shutdown_function('__turbocov_php_src_tests_end');
    } catch (\turbocov\CannotCollectException $exception) {
        // Tests with OPcache settings that turbocov refuses. The test still runs with the extension loaded.
        unset($exception);
    }
} else {
    register_shutdown_function('__turbocov_php_src_tests_end');
}
