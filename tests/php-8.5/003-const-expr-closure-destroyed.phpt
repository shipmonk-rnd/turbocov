--TEST--
A closure in a constant expression of code that the engine destroys before stop() keeps its lines
--EXTENSIONS--
turbocov
--SKIPIF--
<?php
if (PHP_VERSION_ID < 80500) {
    die('skip PHP before 8.5 has no closures in constant expressions');
}
?>
--INI--
turbocov.collect_paths={PWD}/../fixtures
--FILE--
<?php
$file = dirname(__DIR__) . '/fixtures/const-expr-closure-destroyed.php';

for ($run = 0; $run < 3; $run++) {
    $total = 0;

    \turbocov\start();

    for ($i = 0; $i < 50; $i++) {
        $total += require $file;
    }

    echo $total, ': ', implode(',', array_keys(\turbocov\stop()[$file])), "\n";
}
?>
--EXPECT--
50: 3,11,8,5
50: 3,11,8,5
50: 3,11,8,5
