--TEST--
The closure of a partial function application gets probes when it runs for the first time
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
$file = dirname(__DIR__) . '/fixtures/partial-application.php';

\turbocov\start();
require $file;
$executed = \turbocov\stop()[$file];

$covered = array_keys($executed);
$uncovered = array_keys(array_diff_key(\turbocov\get_executable_lines([$file])[$file], $executed));
sort($covered);
sort($uncovered);

echo 'covered: ', implode(',', $covered), "\n";
echo 'not covered: ', implode(',', $uncovered), "\n";

$probes = \turbocov\get_status()['installed_probes'];
$never_called('x', 'y');
echo 'probes for the first call: ', \turbocov\get_status()['installed_probes'] - $probes, "\n";

$probes = \turbocov\get_status()['installed_probes'];
$fired = \turbocov\get_status()['fired_probes'];
$never_called('x', 'y');
$never_called('x', 'y');
echo 'probes for the next calls: ', \turbocov\get_status()['installed_probes'] - $probes, "\n";
echo 'fired probes in the next calls: ', \turbocov\get_status()['fired_probes'] - $fired, "\n";

\turbocov\start();
$greet('night');
echo 'lines of the next collection: ', implode(',', array_keys(\turbocov\stop()[$file])), "\n";
?>
--EXPECT--
good morning world
good evening world
a b c
X,Y
covered: 5,8,9,10,12,15,16,20,21,22,23,24
not covered: 
probes for the first call: 1
probes for the next calls: 0
fired probes in the next calls: 0
lines of the next collection: 8,5
