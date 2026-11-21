<?php
function defined_first($a) {
    return \defined('PHP_VERSION_ID')
        && $a !== null
        && 0 !== (\PHP_VERSION_ID & $a['features']);
}
function function_exists_chain() {
    return \function_exists('strlen')
        && \function_exists('strtoupper')
        && \function_exists('count');
}
function constant_left($a) {
    return \PHP_VERSION_ID >= 80100
        && $a;
}
function constant_right($a) {
    return $a
        && \PHP_VERSION_ID >= 80100;
}
function negative_number($a) {
    return $a
        && -1;
}
function match_condition($a, $b) {
    return match (true) {
        $a !== null
        && $b !== null => [
            'a' => $a,
        ],
        default => [],
    };
}
function array_item_condition($a, $b) {
    return json_encode(['both' => ($a !== null
        && $b !== null)
        ? 'yes'
        : 'no']);
}
function static_initializer() {
    static $x = \PHP_VERSION_ID >= 80100
        && \PHP_INT_SIZE >= 4;
    return $x;
}
function arrow_function_body($a) {
    $check = static fn (int $i): bool => $i > 0 && (
        $i < 10 || $i === 99
    );
    return $check($a);
}
function throwing_operand($a, $b) {
    return $a
        && $b();
}
