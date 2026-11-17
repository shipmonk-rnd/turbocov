--TEST--
Generators, closures, traits, match, try/finally and other constructs report their lines
--EXTENSIONS--
turbocov
--INI--
turbocov.collect_paths={PWD}/fixtures
--FILE--
<?php
$report = static function (string $file): void {
    ob_start();
    \turbocov\start();
    (static function () use ($file): void {
        require $file;
    })();
    $executed = \turbocov\stop()[$file];
    ob_end_clean();

    $covered = array_keys($executed);
    $uncovered = array_keys(array_diff_key(\turbocov\get_executable_lines([$file])[$file], $executed));
    sort($covered);
    sort($uncovered);

    echo basename($file), "\n  covered: ", implode(',', $covered), "\n  not covered: ", implode(',', $uncovered), "\n";
};

$report(__DIR__ . '/fixtures/basic.php');
$report(__DIR__ . '/fixtures/constructs.php');
$report(__DIR__ . '/fixtures/edge-cases.php');
?>
--EXPECTF--
basic.php
  covered: 7,12,13,14,22,24,26,39,40
  not covered: 8,15,16,17,19,23,33,35
constructs.php
  covered: 6,11,13,14,16,17,18,19,22,23,26,28,29,35,36,39,40,42,50,53,54,55,56,58,59,60,61,62,63,64,65,68,69,70,71,%r(72,)?%r73,74,75,76,77,78,80,81,82,83,86,88,89,90
  not covered: 27,38,66,84
edge-cases.php
  covered: 20,21,22,23,28,29,39,40,41,48,50,51,55,60,62,63,64,75,76,77,81,86,89,90,91,92,93,95,96,97,100,105,107,108,109,112,113,116,119,124,125,126,127,130,132,134,135,138,141,145,151,152,153,155,156,157,158,160,161,162,163,164,165,167,168,169,170,172,174,175,176,177,179,180,181,183,184,185,186,187,188,189,191,193,194,195,199,200,201,204,205,207,211,214,215,216
  not covered: 69,70
