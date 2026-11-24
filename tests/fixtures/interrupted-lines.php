<?php
function return_type_check($a): string {
    return $a;
}
function missing_return($a): string {
    if ($a) {
        return 'x';
    }
}
function return_before_finally() {
    try {
        return 1;
    } finally {
        throw new LogicException('finally');
    }
}
function end_of_try() {
    try {
        echo '';
    } finally {
        throw new LogicException('finally');
    }
}
function anonymous_class() {
    return new class extends InterruptedLinesMissingParent {
    };
}

function typed_coalesce($a, $b): string {
    return $a
        ?? $b;
}
function typed_match($a): string {
    return match ($a) {
        1 => 'one',
        default => 'other',
    };
}
