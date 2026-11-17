<?php declare(strict_types = 1);

// Prints the statistics of one suite from the file of raw times.
// Usage: php stats.php <times.tsv> <suite>

[, $file, $suite] = $argv;

$times = [];

foreach (file($file, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES) as $row) {
    [$rowSuite, $round, $mode, $seconds] = explode("\t", $row);

    if ($rowSuite === $suite) {
        $times[$mode][(int) $round] = (float) $seconds;
    }
}

$mean = static fn (array $values): float => array_sum($values) / count($values);
$median = static function (array $values): float {
    sort($values);
    $middle = intdiv(count($values), 2);

    return count($values) % 2 === 1 ? $values[$middle] : ($values[$middle - 1] + $values[$middle]) / 2;
};
$seconds = static fn (float $value): string => number_format($value, 2, '.', '');
$signed = static fn (float $value): string => ($value < 0 ? '-' : '+') . number_format(abs($value), 2, '.', '');

echo "Time of the suite in seconds, " . count($times['none']) . " rounds:\n\n";
echo "| Mode | Mean | Median | Min | Max |\n|---|---|---|---|---|\n";

foreach ($times as $mode => $values) {
    printf("| %s | %s | %s | %s | %s |\n", $mode, $seconds($mean($values)), $seconds($median($values)), $seconds(min($values)), $seconds(max($values)));
}

// The added time of a round is the time of the mode minus the time of none in the same round.
$added = [];

foreach ($times as $mode => $values) {
    if ($mode !== 'none') {
        foreach ($values as $round => $value) {
            $added[$mode][$round] = $value - $times['none'][$round];
        }
    }
}

$ratio = static function (float $value, float $turbocov): string {
    return $turbocov > 0 ? number_format($value / $turbocov, 1, '.', '') : 'n/a';
};

echo "\nTime that the mode adds to none, in seconds:\n\n";
echo "| Mode | Mean | Median | Min | Max | Mean / turbocov | Median / turbocov |\n|---|---|---|---|---|---|---|\n";

foreach ($added as $mode => $values) {
    printf(
        "| %s | %s | %s | %s | %s | %s | %s |\n",
        $mode,
        $signed($mean($values)),
        $signed($median($values)),
        $signed(min($values)),
        $signed(max($values)),
        $mode === 'turbocov' ? '' : $ratio($mean($values), $mean($added['turbocov'])),
        $mode === 'turbocov' ? '' : $ratio($median($values), $median($added['turbocov'])),
    );
}
