<?php declare(strict_types = 1);

namespace Turbocov\Benchmark;

use LogicException;
use PHPUnit\Event\Test\Finished;
use PHPUnit\Event\Test\FinishedSubscriber;
use PHPUnit\Event\Test\PreparationStarted;
use PHPUnit\Event\Test\PreparationStartedSubscriber;
use PHPUnit\Event\TestRunner\ExecutionFinished;
use PHPUnit\Event\TestRunner\ExecutionFinishedSubscriber;
use PHPUnit\Event\TestRunner\ExecutionStarted;
use PHPUnit\Event\TestRunner\ExecutionStartedSubscriber;
use PHPUnit\Runner\Extension\Extension;
use PHPUnit\Runner\Extension\Facade;
use PHPUnit\Runner\Extension\ParameterCollection;
use PHPUnit\TextUI\Configuration\Configuration;
use SebastianBergmann\FileIterator\Facade as FileIterator;
use function array_values;
use function extension_loaded;
use function getenv;
use function realpath;

/**
 * Calls one coverage driver around each test, without the coverage library of PHPUnit.
 *
 * Environment:
 * - TURBOCOV_BENCHMARK_MODE: none, turbocov, pcov or xdebug.
 * - TURBOCOV_BENCHMARK_SOURCE: the source directory of the project.
 * - TURBOCOV_BENCHMARK_CHECK: optional file. The extension then counts the line data and writes the result to it.
 */
final class DriverOnlyExtension implements Extension
{

    private const EXTENSIONS = ['turbocov', 'pcov', 'xdebug'];

    public function bootstrap(
        Configuration $configuration,
        Facade $facade,
        ParameterCollection $parameters,
    ): void
    {
        $mode = (string) getenv('TURBOCOV_BENCHMARK_MODE');
        $source = realpath((string) getenv('TURBOCOV_BENCHMARK_SOURCE'));
        $checkFile = getenv('TURBOCOV_BENCHMARK_CHECK');

        if ($source === false) {
            throw new LogicException('TURBOCOV_BENCHMARK_SOURCE is not a directory');
        }

        $driver = match ($mode) {
            'none' => new NoCalls(),
            'turbocov' => new TurbocovCalls(),
            'pcov' => new PcovCalls(),
            'xdebug' => new XdebugCalls(),
            default => throw new LogicException('TURBOCOV_BENCHMARK_MODE must be none, turbocov, pcov or xdebug'),
        };

        foreach (self::EXTENSIONS as $extension) {
            if (extension_loaded($extension) !== ($extension === $mode)) {
                throw new LogicException("The mode {$mode} needs exactly its own coverage extension, check {$extension}");
            }
        }

        $collector = new Collector($mode, $driver, $checkFile === false || $checkFile === '' ? null : $checkFile);

        $facade->registerSubscribers(
            // PHPUnit creates its coverage driver at this point: after it loaded the tests, before the first test.
            new class ($collector, $source) implements ExecutionStartedSubscriber {

                public function __construct(
                    private readonly Collector $collector,
                    private readonly string $source,
                )
                {
                }

                public function notify(ExecutionStarted $event): void
                {
                    // The same list that PHPUnit makes from <source><include><directory> for the filter of its drivers.
                    $files = (new FileIterator())->getFilesAsArray($this->source, '.php');

                    $this->collector->prepare(array_values($files));
                }

            },
            new class ($collector) implements PreparationStartedSubscriber {

                public function __construct(
                    private readonly Collector $collector,
                )
                {
                }

                public function notify(PreparationStarted $event): void
                {
                    $this->collector->start();
                }

            },
            new class ($collector) implements FinishedSubscriber {

                public function __construct(
                    private readonly Collector $collector,
                )
                {
                }

                public function notify(Finished $event): void
                {
                    $this->collector->stop($event->test()->id());
                }

            },
            new class ($collector) implements ExecutionFinishedSubscriber {

                public function __construct(
                    private readonly Collector $collector,
                )
                {
                }

                public function notify(ExecutionFinished $event): void
                {
                    $this->collector->stop('');
                    $this->collector->writeCheckFile();
                }

            },
        );
    }

}
