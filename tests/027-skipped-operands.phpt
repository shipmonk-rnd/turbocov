--TEST--
A line of a multi-line condition, ternary or match is reported only when its code runs
--EXTENSIONS--
turbocov
--INI--
turbocov.collect_paths={PWD}/fixtures/skipped-operands.php:{PWD}/fixtures/skipped-operands-namespace.php
zend.assertions=1
--FILE--
<?php
require __DIR__ . '/fixtures/skipped-operands.php';
require __DIR__ . '/fixtures/skipped-operands-namespace.php';

$skippedLines = static function (string $label, callable $call): string {
    $function = new ReflectionFunction(explode(' ', $label)[0]);
    $file = $function->getFileName();
    $executable = array_keys(\turbocov\get_executable_lines([$file])[$file]);
    sort($executable);

    \turbocov\start();
    $call();
    $executed = \turbocov\stop()[$file] ?? [];

    $skipped = [];

    foreach ($executable as $line) {
        if ($line > $function->getStartLine() && $line < $function->getEndLine() && !isset($executed[$line])) {
            $skipped[] = $line - $function->getStartLine();
        }
    }

    return $skipped === [] ? 'all lines' : 'not lines +' . implode(' +', $skipped);
};

$report = static function (string $label, callable $call) use ($skippedLines): void {
    echo $label, ': ', $skippedLines($label, $call), "\n";
};

$report('or_chain first', static fn () => or_chain(true, false, false));
$report('or_chain second', static fn () => or_chain(false, true, false));
$report('or_chain none', static fn () => or_chain(false, false, false));
$report('and_return first', static fn () => and_return(false, true));
$report('ternary then', static fn () => ternary(true));
$report('ternary else', static fn () => ternary(false));
$report('coalesce left', static fn () => coalesce('a'));
$report('coalesce right', static fn () => coalesce(null));
$report('short_ternary_chain first', static fn () => short_ternary_chain('a', 'b'));
$report('short_ternary_chain last', static fn () => short_ternary_chain('', ''));
$report('match_arms first', static fn () => match_arms(1));
$report('match_arms default', static fn () => match_arms(3));
$report('nullsafe null', static fn () => nullsafe(null));
$report('nullsafe object', static fn () => nullsafe(new ArrayObject([])));
$report('while_condition first', static fn () => while_condition(false));
$report('array_item left', static fn () => array_item('a'));
$report('method_argument left', static fn () => method_argument('a'));
$report('call_in_condition left', static fn () => call_in_condition('a'));
$report('special_function left', static fn () => special_function('a'));
$report('concat then', static fn () => concat(true));
$report('nested left', static fn () => nested(true, 'b'));
$report('list_assign then', static fn () => list_assign(true));
$report('and_in_if first', static fn () => and_in_if(false, true, true));
$report('else_if first', static fn () => else_if(true, false, false));
$report('throwing_arm first', static fn () => throwing_arm(1));
$report('enum_arms first', static fn () => enum_arms(SkippedOperandsSuit::Hearts));
$report('spread_match first', static fn () => spread_match([0], 1));
$report('SkippedOperands\\assertion first', static fn () => SkippedOperands\assertion(true, false));
$report('SkippedOperands\\function_call left', static fn () => SkippedOperands\function_call('a'));
$report('fallback_throw left', static fn () => fallback_throw('a'));
$report('multi_line_else then', static fn () => multi_line_else(true, 2));
$report('inner_ternary left', static fn () => inner_ternary('a', true));

echo "Lines with other code after the skipped operand stay reported:\n";
$report('break_after then', static fn () => break_after(true));
$report('operand_after left', static fn () => operand_after(1, 2));
$report('variable_argument left', static fn () => variable_argument(1, 2));
$report('constant_argument left', static fn () => constant_argument(1));
$report('statement_after left', static fn () => statement_after(1));
$report('taken second', static fn () => taken(false, true));
$report('equal_after first', static fn () => equal_after(true, false));
$report('equal_false_after first', static fn () => equal_false_after(true, false));
$report('identical_after first', static fn () => identical_after(true, false, true));
$report('multiply_after left', static fn () => multiply_after(2, 3));
$report('plus_after left', static fn () => plus_after(5, 1));
$report('concat_after left', static fn () => concat_after('a', 'b'));
$report('call_after left', static fn () => call_after('a'));
$report('next_argument left', static fn () => next_argument(1, 2, 3));
$report('assignment_line left', static fn () => assignment_line('a'));
$report('SkippedOperands\\next_argument left', static fn () => SkippedOperands\next_argument(1, 2, 3));

// Before PHP 8.5, "!X" and "X == false" have the same opcode, and "(bool) X" and "X == true" too. The line then stays reported.
$expected = PHP_VERSION_ID >= 80500 ? 'not lines +2' : 'all lines';

foreach (['negation first' => static fn () => negation(true, false), 'bool_cast left' => static fn () => bool_cast('a', 'b')] as $label => $call) {
    $result = $skippedLines($label, $call);
    echo $label, ': ', $result === $expected ? 'correct for this PHP version' : $result, "\n";
}
?>
--EXPECT--
or_chain first: not lines +2 +3 +7
or_chain second: not lines +3 +7
or_chain none: not lines +5
and_return first: not lines +2
ternary then: not lines +3
ternary else: not lines +2
coalesce left: not lines +2
coalesce right: all lines
short_ternary_chain first: not lines +2 +3
short_ternary_chain last: all lines
match_arms first: not lines +3 +4
match_arms default: not lines +2 +3
nullsafe null: not lines +2 +3
nullsafe object: all lines
while_condition first: not lines +3 +5
array_item left: not lines +3
method_argument left: not lines +3
call_in_condition left: not lines +3 +7
special_function left: not lines +2
concat then: not lines +3
nested left: not lines +3 +4
list_assign then: not lines +3
and_in_if first: not lines +2 +3 +5
else_if first: not lines +2 +4 +5 +9
throwing_arm first: not lines +3
enum_arms first: not lines +3
spread_match first: not lines +5
SkippedOperands\assertion first: not lines +2
SkippedOperands\function_call left: not lines +2
fallback_throw left: not lines +2
multi_line_else then: not lines +4 +5
inner_ternary left: not lines +2 +3 +4
Lines with other code after the skipped operand stay reported:
break_after then: all lines
operand_after left: all lines
variable_argument left: all lines
constant_argument left: all lines
statement_after left: all lines
taken second: all lines
equal_after first: all lines
equal_false_after first: all lines
identical_after first: all lines
multiply_after left: all lines
plus_after left: all lines
concat_after left: all lines
call_after left: all lines
next_argument left: all lines
assignment_line left: all lines
SkippedOperands\next_argument left: all lines
negation first: correct for this PHP version
bool_cast left: correct for this PHP version
