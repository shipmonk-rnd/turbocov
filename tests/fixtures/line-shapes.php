<?php
function return_type_error(): array {
    return 1;
}
function missing_return($a): int {
    $a++;
}
function return_in_try() {
    try {
        return 1;
    } finally {
        throw new RuntimeException('finally');
    }
}
function break_in_try($a) {
    foreach ($a as $item) {
        try {
            break;
        } finally {
            throw new RuntimeException('finally');
        }
    }
    return 0;
}
function empty_try() {
    try {
    } finally {
        throw new RuntimeException('finally');
    }
}
function class_declaration() {
    try {
        class LineShapesChild extends LineShapesMissingParent {}
    } catch (Error $e) {
        return 1;
    }
    return 0;
}
function anonymous_class() {
    return new class extends LineShapesMissingParent {
    };
}
function duplicate_arm($a) {
    echo match ($a) {
        1 => 'one',
        2 => 'two',
        1 => 'again',
    };
}
function throws(): never {
    throw new RuntimeException('never');
}
function after_never() {
    throws();
}
function after_finally($a) {
    try {
        strlen($a);
    } finally {
        throw new RuntimeException('finally');
    }
    return 2;
}
function assertion($a) {
    assert($a);
    return 1;
}
function one_line(int $a) { return $a; }
function first_arm($a) {
    return match ($a) {
        null => 'null',
        false => 'false',
        0 => 'int',
        default => 'other',
    };
}
function call_end() {
    return array_map(function ($item) {
        throw new RuntimeException('item');
    }, [1]);
}
function closure_end() {
    $closure = function ($item) {
        if ($item > 0) {
            throw new RuntimeException('positive');
        }
    };
    return $closure(1);
}
function immediate_call() {
    return (function () {
        return 1;
    })();
}
function sum($a, $b, $c) {
    return $a + $b + $c;
}
function not_listed(
    $a,
    $b = 2
) {
    global $lineShapes;
    return sum(
        $a,
        3,
        $b
    );
}
