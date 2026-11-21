<?php
namespace SkippedOperands;

function assertion($a, $b) {
    assert($a
        || $b);
    return 1;
}
function function_call($a) {
    return strlen($a
        ?? 'none');
}

function next_argument($a, $b, $c) {
    return max($a
        ?? $b, $c);
}
