--TEST--
A file that is not selected keeps its jump tables, and a selected file reports each case line
--EXTENSIONS--
turbocov
--INI--
turbocov.collect_paths={PWD}/fixtures/switch.php
--FILE--
<?php
$before = \turbocov\get_status()['jump_table_files'];

require __DIR__ . '/fixtures/functions.php';
echo 'not selected: ', \turbocov\get_status()['jump_table_files'] - $before, "\n";

$before = \turbocov\get_status()['jump_table_files'];
$file = __DIR__ . '/fixtures/switch.php';

\turbocov\start();
require $file;
zone('uk');
$executed = array_keys(\turbocov\stop()[$file]);
sort($executed);

echo 'selected: ', \turbocov\get_status()['jump_table_files'] - $before, "\n";
$executable = array_keys(\turbocov\get_executable_lines([$file])[$file]);
sort($executable);

echo 'executable: ', implode(',', $executable), "\n";
echo 'executed: ', implode(',', $executed), "\n";
?>
--EXPECT--
not selected: 1
selected: 0
executable: 6,7,8,9,10,11,12,13,14,15,17,20
executed: 6,8,10,11,20
