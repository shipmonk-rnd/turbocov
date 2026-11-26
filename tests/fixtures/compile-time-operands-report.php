<?php
require __DIR__ . '/compile-time-operands.php';

$report = static function (string $label, callable $call): void {
    $function = new ReflectionFunction(explode(' ', $label)[0]);
    $file = $function->getFileName();
    $executable = array_keys(\turbocov\get_executable_lines([$file])[$file]);
    sort($executable);

    \turbocov\start();

    try {
        $call();
    } catch (LogicException) {
    }

    $executed = \turbocov\stop()[$file] ?? [];
    $lines = ['executed' => [], 'not executed' => []];

    foreach ($executable as $line) {
        if ($line > $function->getStartLine() && $line < $function->getEndLine()) {
            $lines[isset($executed[$line]) ? 'executed' : 'not executed'][] = '+' . ($line - $function->getStartLine());
        }
    }

    echo $label, ': ';

    foreach (array_filter($lines) as $status => $offsets) {
        echo $status, ' ', implode(' ', $offsets), $status === 'executed' && $lines['not executed'] !== [] ? ', ' : '';
    }

    echo "\n";
};

$throw = static fn () => throw new LogicException('none');

echo "All operands run:\n";
$report('defined_first all', static fn () => defined_first(['features' => 1]));
$report('function_exists_chain all', static fn () => function_exists_chain());
$report('constant_left all', static fn () => constant_left(true));
$report('constant_right all', static fn () => constant_right(true));
$report('negative_number all', static fn () => negative_number(true));
$report('match_condition all', static fn () => match_condition(1, 2));
$report('array_item_condition all', static fn () => array_item_condition(1, 2));
$report('static_initializer all', static fn () => static_initializer());
$report('arrow_function_body all', static fn () => arrow_function_body(99));
$report('throwing_operand all', static fn () => throwing_operand(true, $throw));

echo "PHP skips an operand:\n";
$report('defined_first first', static fn () => defined_first(null));
$report('constant_left first', static fn () => constant_left(false));
$report('constant_right first', static fn () => constant_right(false));
$report('negative_number first', static fn () => negative_number(false));
$report('match_condition first', static fn () => match_condition(null, 2));
$report('array_item_condition first', static fn () => array_item_condition(null, 2));
$report('arrow_function_body first', static fn () => arrow_function_body(0));
$report('arrow_function_body second', static fn () => arrow_function_body(5));
$report('throwing_operand first', static fn () => throwing_operand(false, $throw));
