<?php declare(strict_types = 1);

namespace Turbocov\Benchmark;

final class NoCalls implements DriverCalls
{

    public function prepare(array $sourceFiles): void
    {
    }

    public function start(): void
    {
    }

    public function stop(): array
    {
        return [];
    }

}
