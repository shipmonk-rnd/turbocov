--TEST--
Property hooks report their lines
--EXTENSIONS--
turbocov
--SKIPIF--
<?php
if (PHP_VERSION_ID < 80400) {
    die('skip PHP before 8.4 has no property hooks');
}
?>
--INI--
turbocov.collect_paths={PWD}/../fixtures
--FILE--
<?php
$file = dirname(__DIR__) . '/fixtures/property-hooks.php';

ob_start();
\turbocov\start();
require $file;
$executed = \turbocov\stop()[$file];
ob_end_clean();

$covered = array_keys($executed);
$uncovered = array_keys(array_diff_key(\turbocov\get_executable_lines([$file])[$file], $executed));
sort($covered);
sort($uncovered);

echo 'covered: ', implode(',', $covered), "\n";
echo 'not covered: ', implode(',', $uncovered), "\n";
echo implode("\n", array_column(\turbocov\get_debug_info($file)['units'], 'name')), "\n";
?>
--EXPECT--
covered: 7,10,14,15,19,20,30,31,32,33
not covered: 11,25
Temperature::$celsius::get
Temperature::$celsius::set
Temperature::$fahrenheit::get
Temperature::$unused::get
