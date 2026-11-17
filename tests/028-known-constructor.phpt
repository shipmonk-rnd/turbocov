--TEST--
The arguments of "new" report the lines that PCOV reports, also when PHP knows the constructor at compile time
--EXTENSIONS--
turbocov
--INI--
turbocov.collect_paths={PWD}/fixtures
--FILE--
<?php
$file = __DIR__ . '/fixtures/known-constructor.php';
require $file;

$lines = array_keys(\turbocov\get_executable_lines([$file])[$file]);
sort($lines);

// PHP 8.6 knows the constructor of an internal class at compile time. It then emits SEND_VAL and SEND_VAR
// where older versions emit SEND_VAL_EX and SEND_VAR_EX. PCOV ignores SEND_VAL and SEND_VAR_EX.
$expected = PHP_VERSION_ID >= 80600 ? [3, 4, 5, 7, 9, 11] : [3, 4, 5, 6, 7, 8, 11];

echo $lines === $expected ? 'the lines of PCOV' : implode(',', $lines), "\n";
?>
--EXPECT--
the lines of PCOV
