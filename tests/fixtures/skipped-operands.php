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
