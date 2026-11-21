--TEST--
A line of a multi-line condition, ternary or match is reported only when its code runs
--EXTENSIONS--
turbocov
--INI--
turbocov.collect_paths={PWD}/fixtures/skipped-operands.php
--FILE--
<?php
$file = __DIR__ . '/fixtures/skipped-operands.php';
require $file;

$executable = array_keys(\turbocov\get_executable_lines([$file])[$file]);
sort($executable);

$report = static function (string $label, callable $call) use ($file, $executable): void {
    $function = new ReflectionFunction(explode(' ', $label)[0]);

    \turbocov\start();
    $call();
    $executed = \turbocov\stop()[$file] ?? [];

    $skipped = [];

    foreach ($executable as $line) {
        if ($line > $function->getStartLine() && $line < $function->getEndLine() && !isset($executed[$line])) {
            $skipped[] = $line - $function->getStartLine();
        }
    }

    echo $label, ': ', $skipped === [] ? 'all lines' : 'not lines +' . implode(' +', $skipped), "\n";
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

echo "Lines with other code after the skipped operand stay reported:\n";
$report('break_after then', static fn () => break_after(true));
$report('operand_after left', static fn () => operand_after(1, 2));
$report('variable_argument left', static fn () => variable_argument(1, 2));
$report('constant_argument left', static fn () => constant_argument(1));
$report('statement_after left', static fn () => statement_after(1));
$report('taken second', static fn () => taken(false, true));
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
Lines with other code after the skipped operand stay reported:
break_after then: all lines
operand_after left: all lines
variable_argument left: all lines
constant_argument left: all lines
statement_after left: all lines
taken second: all lines
