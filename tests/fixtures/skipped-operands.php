<?php
function or_chain($a, $b, $c) {
    if ($a
        || $b
        || $c
    ) {
        return 1;
    }
    return 0;
}
function and_return($a, $b) {
    return $a
        && $b;
}
function ternary($a) {
    return $a
        ? 'then'
        : 'else';
}
function coalesce($a) {
    return $a
        ?? strtoupper('b');
}
function short_ternary_chain($a, $b) {
    return $a
        ?: $b
        ?: 'c';
}
function match_arms($a) {
    return match ($a) {
        1 => 'one',
        2 => 'two',
        default => 'other',
    };
}
function nullsafe($a) {
    return $a
        ?->getIterator()
        ?->count();
}
function while_condition($a) {
    $i = 0;
    while ($a
        && $i < 1
    ) {
        $i++;
    }
    return $i;
}
function array_item($a) {
    return [
        'x' => $a
            ?? 'none',
        'y' => 2,
    ];
}
function method_argument($a) {
    return (new ArrayObject([]))->offsetExists(
        $a
            ?? 'none'
    );
}
function call_in_condition($a) {
    if (trim(
        $a
            ?? 'none'
    )) {
        return 1;
    }
    return 0;
}
function special_function($a) {
    return strlen($a
        ?? 'none');
}
function concat($a) {
    return 'x' . ($a
        ? 'then'
        : 'else');
}
function nested($a, $b) {
    return match (true) {
        $a => $b
            ?? 'none',
        default => 'other',
    };
}
function list_assign($a) {
    [$x, $y] = $a
        ? [1, 2]
        : [3, 4];
    return $x + $y;
}

function and_in_if($a, $b, $c) {
    if ($a
        && $b
        && $c
    ) {
        return 1;
    }
    return 0;
}
function else_if($a, $b, $c) {
    if ($a === null) {
        return 0;
    } elseif ($a
        || $b
        || $c
    ) {
        return 1;
    }
    return 2;
}
function throwing_arm($a) {
    return match ($a) {
        1 => 'one',
        default => throw new LogicException('none'),
    };
}
enum SkippedOperandsSuit {
    case Hearts;
    case Spades;
}
function enum_arms(SkippedOperandsSuit $a) {
    return match ($a) {
        SkippedOperandsSuit::Hearts => 'red',
        SkippedOperandsSuit::Spades => 'black',
    };
}
function spread_match($a, $b) {
    return [
        ...$a,
        ...match ($b) {
            1 => ['one'],
            2 => ['two'],
        },
    ];
}
function fallback_throw($a) {
    return $a
        ?: throw new LogicException('none');
}
function multi_line_else($a, $b) {
    return $a
        ? [1]
        : [
            $b,
            3,
        ];
}
function inner_ternary($a, $b) {
    return strtoupper($a ?: (
        $b
            ? 'b'
            : 'c'
    ));
}

function break_after($a) {
    foreach ([1, 2] as $v) {
        $x = $a ? 1
            : 2; break;
    }
    return $x;
}
function operand_after($a, $b) {
    return ($a
        ?? 0) > $b;
}
function variable_argument($a, $b) {
    return max($a
        ?? 0, $b);
}
function constant_argument($a) {
    return (new ArrayObject([]))->offsetSet($a
        ?? 0, 5);
}
function statement_after($a) {
    $x = $a
        ?: 0; $y = 1;
    return $x + $y;
}
function taken($a, $b) {
    return $a
        || $b;
}
function equal_false_after($a, $b) {
    return ($a
        || $b) == false;
}
function equal_after($a, $b) {
    return ($a
        || $b) == true;
}
function identical_after($a, $b, $c) {
    return ($a
        || $b) === $c;
}
function multiply_after($a, $b) {
    return ($a
        ?? 1) * $b;
}
function plus_after($a, $b) {
    return ($a
        ?: 1) + $b;
}
function concat_after($a, $b) {
    return ($a
        ?? 'x') . $b;
}
function call_after($a) {
    return strlen($a
        ?? 'x') + strlen('yy');
}
function next_argument($a, $b, $c) {
    return (new ArrayObject([]))->offsetSet($a
        ?? $b, $c);
}
function assignment_line($a) {
    $x =
        $a ?? 'none';
    return $x;
}

function negation($a, $b) {
    return !($a
        || $b);
}
function bool_cast($a, $b) {
    return (bool) ($a
        ?? $b);
}
