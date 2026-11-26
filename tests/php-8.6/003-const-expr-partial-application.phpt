--TEST--
A closure in an argument of a partial function application in a constant expression reports its lines
--EXTENSIONS--
turbocov
--SKIPIF--
<?php
if (PHP_VERSION_ID < 80600) {
    die('skip PHP before 8.6 has no partial function application');
}
?>
--INI--
turbocov.collect_paths={PWD}/../fixtures
--FILE--
<?php
$file = dirname(__DIR__) . '/fixtures/const-expr-partial-application.php';

\turbocov\start();
require $file;
$function = null;
$executed = \turbocov\stop()[$file];

$covered = array_keys($executed);
$uncovered = array_keys(array_diff_key(\turbocov\get_executable_lines([$file])[$file], $executed));
sort($covered);
sort($uncovered);

echo 'covered: ', implode(',', $covered), "\n";
echo 'not covered: ', implode(',', $uncovered), "\n";
?>
--EXPECT--
class constant
1
covered: 5,16,17,43,45,47,48,50,53,54
not covered: 11,21,26,29,33,37,41
