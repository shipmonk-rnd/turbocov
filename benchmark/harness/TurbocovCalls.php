<?php declare(strict_types = 1);

namespace Turbocov\Benchmark;

use LogicException;
use function array_diff_key;
use function array_keys;
use function array_replace;
use function turbocov\ensure_can_collect;
use function turbocov\get_executable_lines;
use function turbocov\start;
use function turbocov\stop;

/**
 * The calls of ShipMonk\Turbocov\TurbocovDriver in shipmonk/turbocov-phpunit.
 */
final class TurbocovCalls implements DriverCalls
{

    /**
     * @var array<string, true>
     */
    private array $knownFiles = [];

    public function prepare(array $sourceFiles): void
    {
        ensure_can_collect();
    }

    public function start(): void
    {
        start();
    }

    public function stop(): array
    {
        $lines = stop();
        $newFiles = array_diff_key($lines, $this->knownFiles);

        if ($newFiles !== []) {
            $executableLines = get_executable_lines(array_keys($newFiles));

            foreach ($newFiles as $file => $executedLines) {
                if (!isset($executableLines[$file])) {
                    throw new LogicException("turbocov has no executable lines for {$file}");
                }

                $lines[$file] = array_replace($executableLines[$file], $executedLines);
                $this->knownFiles[$file] = true;
            }
        }

        return $lines;
    }

}
