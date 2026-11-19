--TEST--
A second PHP thread turns the collection off and each function reports it
--EXTENSIONS--
turbocov
parallel
--INI--
turbocov.collect_paths={PWD}/../fixtures
--FILE--
<?php
require __DIR__ . '/../fixtures/functions.php';

\turbocov\start();
second(2);

$runtime = new \parallel\Runtime();
$future = $runtime->run(static function (string $file): array {
    $messages = [];

    foreach (['ensure_can_collect', 'start', 'stop', 'get_status'] as $function) {
        try {
            ("turbocov\\$function")();
        } catch (\turbocov\CannotCollectException $e) {
            $messages[] = "thread $function(): " . $e->getMessage();
        }
    }

    // The thread compiles and runs a selected file. Its code gets no probes.
    require $file;
    $messages[] = 'thread second(3): ' . second(3);

    return $messages;
}, [__DIR__ . '/../fixtures/functions.php']);

echo implode("\n", $future->value()), "\n";
$runtime->close();

// The owner thread compiles a file after the second thread started.
eval('function fourth(int $value): int { return first($value) + 1; }');
echo 'fourth(1): ', fourth(1), "\n";

foreach (['stop', 'ensure_can_collect', 'start', 'stop'] as $function) {
    try {
        ("turbocov\\$function")();
        echo $function, "(): no exception\n";
    } catch (\turbocov\CannotCollectException $e) {
        echo $function, '(): ', $e->getMessage(), "\n";
    }
}

$status = \turbocov\get_status();
echo 'collecting: ', json_encode($status['collecting']), "\n";
echo 'disabled_reason: ', $status['disabled_reason'], "\n";
echo 'second(3): ', second(3), "\n";
?>
--EXPECT--
thread ensure_can_collect(): turbocov cannot collect coverage: a second PHP thread started in this process (ext-parallel, FrankenPHP). turbocov supports one PHP thread only
thread start(): turbocov cannot collect coverage: a second PHP thread started in this process (ext-parallel, FrankenPHP). turbocov supports one PHP thread only
thread stop(): turbocov cannot collect coverage: a second PHP thread started in this process (ext-parallel, FrankenPHP). turbocov supports one PHP thread only
thread get_status(): turbocov cannot collect coverage: a second PHP thread started in this process (ext-parallel, FrankenPHP). turbocov supports one PHP thread only
thread second(3): 3
fourth(1): 2
stop(): turbocov cannot collect coverage: a second PHP thread started in this process (ext-parallel, FrankenPHP). turbocov supports one PHP thread only
ensure_can_collect(): turbocov cannot collect coverage: a second PHP thread started in this process (ext-parallel, FrankenPHP). turbocov supports one PHP thread only
start(): turbocov cannot collect coverage: a second PHP thread started in this process (ext-parallel, FrankenPHP). turbocov supports one PHP thread only
stop(): turbocov cannot collect coverage: a second PHP thread started in this process (ext-parallel, FrankenPHP). turbocov supports one PHP thread only
collecting: false
disabled_reason: a second PHP thread started in this process (ext-parallel, FrankenPHP). turbocov supports one PHP thread only
second(3): 3
