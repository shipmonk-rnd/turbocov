--TEST--
A line is reported when PHP reaches it and a throw or a finally block stops it
--EXTENSIONS--
turbocov
--INI--
turbocov.collect_paths={PWD}/fixtures/interrupted-lines.php
--FILE--
<?php
require __DIR__ . '/fixtures/interrupted-lines.php';

$report = static function (string $name, mixed ...$arguments): void {
    $function = new ReflectionFunction($name);
    $file = $function->getFileName();
    $lines = \turbocov\get_executable_lines([$file])[$file];
    ksort($lines);

    \turbocov\start();

    try {
        $name(...$arguments);
        $end = 'returns';
    } catch (Throwable $e) {
        $end = 'throws ' . $e::class;
    }

    $executed = \turbocov\stop()[$file] ?? [];
    $result = ['executed' => '', 'not executed' => ''];

    foreach (array_keys($lines) as $line) {
        if ($line > $function->getStartLine() && $line <= $function->getEndLine()) {
            $result[isset($executed[$line]) ? 'executed' : 'not executed'] .= ' +' . ($line - $function->getStartLine());
        }
    }

    echo $name, ' ', $end, ': ', implode(', ', array_map(static fn ($status) => $status . $result[$status], array_keys(array_filter($result)))), "\n";
};

$report('return_type_check', []);
$report('missing_return', false);
$report('return_before_finally');
$report('end_of_try');
$report('anonymous_class');

echo "A skipped operand before a return type check stays not executed:\n";
$report('typed_coalesce', 'a', 'b');
$report('typed_match', 1);
?>
--EXPECT--
return_type_check throws TypeError: executed +1
missing_return throws TypeError: executed +1 +4, not executed +2
return_before_finally throws LogicException: executed +2 +4
end_of_try throws LogicException: executed +2 +3 +4, not executed +6
anonymous_class throws Error: executed +1
A skipped operand before a return type check stays not executed:
typed_coalesce returns: executed +1, not executed +2
typed_match returns: executed +2, not executed +3
