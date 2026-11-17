<?php declare(strict_types = 1);

namespace Turbocov\Benchmark;

use function array_intersect;
use function pcov\clear;
use function pcov\collect;
use function pcov\start;
use function pcov\stop;
use function pcov\waiting;
use const pcov\inclusive;

/**
 * The calls of SebastianBergmann\CodeCoverage\Driver\PcovDriver.
 */
final class PcovCalls implements DriverCalls
{

    /**
     * @var list<string>
     */
    private array $sourceFiles = [];

    public function prepare(array $sourceFiles): void
    {
        $this->sourceFiles = $sourceFiles;
    }

    public function start(): void
    {
        start();
    }

    public function stop(): array
    {
        stop();

        $filesToCollectCoverageFor = waiting();
        $collected = [];

        if ($filesToCollectCoverageFor !== []) {
            if ($this->sourceFiles !== []) {
                $filesToCollectCoverageFor = array_intersect($filesToCollectCoverageFor, $this->sourceFiles);
            }

            $collected = collect(inclusive, $filesToCollectCoverageFor);

            clear();
        }

        return $collected;
    }

}
