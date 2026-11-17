<?php declare(strict_types = 1);

// Reads the check files of one suite. Fails when a driver gave no line data, or when the baseline had a driver.
// Usage: php check.php <check file of each mode>...

$failed = false;

foreach (array_slice($argv, 1) as $file) {
    $check = json_decode((string) file_get_contents($file), true, flags: JSON_THROW_ON_ERROR);
    $mode = $check['mode'];
    $loaded = implode(', ', array_keys(array_filter($check['loaded']))) ?: 'no coverage extension';

    printf(
        "%-9s loaded: %s; %d of %d tests gave executed lines; %d files; %d executed lines in all tests",
        $mode . ':',
        $loaded,
        $check['tests_with_executed_lines'],
        $check['tests'],
        $check['files_with_data'],
        $check['executed_lines_of_all_tests'],
    );

    if ($check['xdebug_flags'] !== null) {
        echo "; flags {$check['xdebug_flags']}";
    }

    if ($check['turbocov_status'] !== null) {
        echo "; {$check['turbocov_status']['fired_probes']} fired probes";
    }

    if ($check['late_stops'] > 0) {
        echo "; {$check['late_stops']} tests stopped late";
    }

    echo "\n";

    if (($mode === 'none') !== ($check['tests_with_executed_lines'] === 0)) {
        fwrite(STDERR, "The mode {$mode} has a wrong amount of line data\n");
        $failed = true;
    }
}

exit($failed ? 1 : 0);
