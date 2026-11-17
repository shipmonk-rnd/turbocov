<?php declare(strict_types = 1);

spl_autoload_register(static function (string $class): void {
    $prefix = 'Turbocov\\Benchmark\\';

    if (str_starts_with($class, $prefix)) {
        require __DIR__ . '/' . substr($class, strlen($prefix)) . '.php';
    }
});
