<?php declare(strict_types=1);

// Compares the line data of PCOV and turbocov for one file. Input: the JSON of collect.php for PCOV, then the same for turbocov, one on each line.
// PCOV can report more executed lines: it also reports the line of an operand that PHP skips in a multi-line expression. That is not an error.

$file = $argv[1];
$pcov = json_decode((string) fgets(STDIN), true, 512, JSON_THROW_ON_ERROR);
$turbocov = json_decode((string) fgets(STDIN), true, 512, JSON_THROW_ON_ERROR);

$executed = static fn (array $lines): array => array_keys($lines, 1, true);

$errors = array_filter([
    'executable only with PCOV' => array_keys(array_diff_key($pcov, $turbocov)),
    'executable only with turbocov' => array_keys(array_diff_key($turbocov, $pcov)),
    'executed only with turbocov' => array_values(array_diff($executed($turbocov), $executed($pcov))),
]);
$skipped = array_values(array_diff($executed($pcov), $executed($turbocov)));

if ($errors !== []) {
    echo "DIFFERENT $file\n";

    foreach ($errors as $problem => $lines) {
        echo "  $problem: ", implode(', ', $lines), "\n";
    }

    exit(1);
}

echo $skipped === [] ? 'same     ' : 'stricter ', " $file\n";

if ($skipped !== []) {
    echo '  executed only with PCOV: ', implode(', ', $skipped), "\n";
}
