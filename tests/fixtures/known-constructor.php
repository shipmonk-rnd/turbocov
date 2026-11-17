<?php declare(strict_types=1);

$flags = 0;
$fiber = new Fiber(function (): int {
    return 1;
});
$list = new ArrayObject(
    [],
    $flags,
);
