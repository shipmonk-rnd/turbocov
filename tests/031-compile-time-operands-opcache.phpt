--TEST--
With OPcache on, an executed line of a multi-line && is reported as executed when PHP evaluates an operand at compile time
--EXTENSIONS--
turbocov
--SKIPIF--
<?php
if (!extension_loaded('Zend OPcache')) {
    die('skip OPcache is not loaded');
}
?>
--INI--
opcache.enable=1
opcache.enable_cli=1
opcache.jit=disable
opcache.optimization_level=0
opcache.protect_memory=0
opcache.file_update_protection=0
turbocov.collect_paths={PWD}/fixtures/compile-time-operands.php
--FILE--
<?php
require __DIR__ . '/fixtures/compile-time-operands-report.php';
var_dump(opcache_is_script_cached(__DIR__ . '/fixtures/compile-time-operands.php'));
?>
--EXPECT--
All operands run:
defined_first all: executed +2 +3
function_exists_chain all: executed +1 +2 +3
constant_left all: executed +2
constant_right all: executed +1 +2
negative_number all: executed +1 +2
match_condition all: executed +1 +4, not executed +6
array_item_condition all: executed +1 +3, not executed +4
static_initializer all: executed +1 +3
arrow_function_body all: executed +1 +2 +4
throwing_operand all: executed +1 +2
PHP skips an operand:
defined_first first: executed +2, not executed +3
constant_left first: executed +2
constant_right first: executed +1, not executed +2
negative_number first: executed +1, not executed +2
match_condition first: executed +1 +6, not executed +4
array_item_condition first: executed +1 +4, not executed +3
arrow_function_body first: executed +1 +4, not executed +2
arrow_function_body second: executed +1 +2 +4
throwing_operand first: executed +1, not executed +2
bool(true)
