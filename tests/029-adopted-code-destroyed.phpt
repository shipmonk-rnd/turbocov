--TEST--
Code that gets its probes when it runs for the first time, and that the engine destroys, leaves no unit
--EXTENSIONS--
turbocov
--INI--
turbocov.collect_paths={PWD}
--FILE--
<?php
/*
 * PHP 8.5 keeps the closure of a constant expression in a place that the compile hook does not read.
 * Each call of the outer closure makes a copy of that code, and only the copy gets the unit.
 * PHP 8.4 cannot parse such a closure, thus it runs code that the compile hook reads.
 */
$code = PHP_VERSION_ID >= 80500
    ? '$function = function ($default = static function () { return 1; }) { return $default(); }; return $function();'
    : '$function = function () { return 1; }; return $function();';

function run(string $code): int
{
    $total = 0;

    \turbocov\start();

    for ($i = 0; $i < 50; $i++) {
        $total += eval($code);
    }

    \turbocov\stop();

    return $total;
}

echo run($code), "\n";
$units = \turbocov\get_status()['live_units'];

echo run($code), "\n";
echo run($code), "\n";
echo 'new units: ', \turbocov\get_status()['live_units'] - $units, "\n";
?>
--EXPECT--
50
50
50
new units: 0
