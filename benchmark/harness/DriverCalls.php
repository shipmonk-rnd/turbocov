<?php declare(strict_types = 1);

namespace Turbocov\Benchmark;

/**
 * The calls that a coverage driver class of PHPUnit makes to its extension.
 */
interface DriverCalls
{

    /**
     * @param list<string> $sourceFiles
     */
    public function prepare(array $sourceFiles): void;

    public function start(): void;

    /**
     * @return array<string, array<int, int>>
     */
    public function stop(): array;

}
