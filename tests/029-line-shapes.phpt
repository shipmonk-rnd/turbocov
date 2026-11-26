--TEST--
Line shapes with a known result: where the report differs from what PHP executes, and where it is exact
--EXTENSIONS--
turbocov
--INI--
turbocov.collect_paths={PWD}/fixtures/line-shapes.php
zend.assertions=1
--FILE--
<?php
require __DIR__ . '/fixtures/line-shapes.php';

// Prints each executable line of the function and its result. Line +0 is the line with "function".
// The label names the line that the case is about. The other lines are there only for reference.
$report = static function (string $label, callable $call): void {
    $function = new ReflectionFunction(explode(' ', $label)[0]);
    $file = $function->getFileName();
    $end = 'returns';

    ob_start();
    \turbocov\start();

    try {
        $call();
    } catch (Throwable $e) {
        $end = 'throws ' . $e::class;
    }

    $executed = \turbocov\stop()[$file] ?? [];
    ob_end_clean();

    $lines = [];

    foreach (array_keys(\turbocov\get_executable_lines([$file])[$file]) as $line) {
        if ($line >= $function->getStartLine() && $line <= $function->getEndLine()) {
            $lines[$line] = '+' . ($line - $function->getStartLine()) . (isset($executed[$line]) ? ' executed' : ' not executed');
        }
    }

    ksort($lines);
    echo $label, ', ', $end, ': ', implode(', ', $lines), "\n";
};

echo "PHP reaches the line, the report has it as not executed:\n";
$report('class_declaration +2', static fn () => class_declaration());
$report('call_end +3', static fn () => call_end());

echo "PHP cannot reach the line, the report has it as executed or as executable:\n";
$report('duplicate_arm +4', static fn () => duplicate_arm(1));
$report('after_never +2', static fn () => after_never());
$report('after_finally +6', static fn () => after_finally('a'));

echo "PHP starts the line and does not run its main part:\n";
$report('assertion +1', static function () {
    ini_set('zend.assertions', '0');
    assertion(false);
    ini_set('zend.assertions', '1');
});
$report('one_line +0', static fn () => one_line('a'));

echo "The condition of the first arm has the line of \"match\", so the arm is not executed when PHP compares it:\n";
$report('first_arm +2', static fn () => first_arm(0));

echo "The report is exact:\n";
$report('return_type_error +1', static fn () => return_type_error());
$report('missing_return +2', static fn () => missing_return(1));
$report('return_in_try +2', static fn () => return_in_try());
$report('break_in_try +3', static fn () => break_in_try([1]));
$report('empty_try +2', static fn () => empty_try());
$report('anonymous_class +1', static fn () => anonymous_class());
$report('closure_end +5', static fn () => closure_end());
$report('immediate_call +3', static fn () => immediate_call());

echo "A line with only a parameter, a constant argument or a global statement is not an executable line:\n";
$report('not_listed +1 +2 +4 +7', static fn () => not_listed(1));
?>
--EXPECT--
PHP reaches the line, the report has it as not executed:
class_declaration +2, returns: +2 not executed, +3 executed, +4 executed, +6 not executed
call_end +3, throws RuntimeException: +1 executed, +2 executed, +3 not executed
PHP cannot reach the line, the report has it as executed or as executable:
duplicate_arm +4, returns: +1 executed, +2 executed, +3 not executed, +4 executed, +6 executed
after_never +2, throws RuntimeException: +1 executed, +2 not executed
after_finally +6, throws RuntimeException: +2 executed, +3 executed, +4 executed, +6 not executed
PHP starts the line and does not run its main part:
assertion +1, returns: +1 not executed, +2 executed
one_line +0, throws TypeError: +0 not executed
The condition of the first arm has the line of "match", so the arm is not executed when PHP compares it:
first_arm +2, returns: +1 executed, +2 not executed, +3 executed, +4 executed, +5 not executed
The report is exact:
return_type_error +1, throws TypeError: +1 executed
missing_return +2, throws TypeError: +1 executed, +2 executed
return_in_try +2, throws RuntimeException: +2 executed, +4 executed
break_in_try +3, throws RuntimeException: +1 executed, +3 executed, +5 executed, +8 not executed
empty_try +2, throws RuntimeException: +2 executed, +3 executed, +5 not executed
anonymous_class +1, throws Error: +1 executed
closure_end +5, throws RuntimeException: +1 executed, +2 executed, +3 executed, +5 not executed, +6 executed
immediate_call +3, returns: +1 executed, +2 executed, +3 executed
A line with only a parameter, a constant argument or a global statement is not an executable line:
not_listed +1 +2 +4 +7, returns: +5 executed, +6 executed, +8 executed
