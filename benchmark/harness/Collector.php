<?php declare(strict_types = 1);

namespace Turbocov\Benchmark;

use SebastianBergmann\CodeCoverage\Version;
use function array_keys;
use function count;
use function extension_loaded;
use function file_put_contents;
use function json_encode;
use function turbocov\get_status;
use const JSON_PRETTY_PRINT;
use const JSON_THROW_ON_ERROR;
use const JSON_UNESCAPED_SLASHES;

/**
 * Starts and stops the driver around each test. Drops the line data, or counts it when a check file is set.
 */
final class Collector
{

    private bool $running = false;

    private int $tests = 0;

    private int $testsWithExecutedLines = 0;

    private int $executedLines = 0;

    private int $lateStops = 0;

    private int $sourceFiles = 0;

    /**
     * @var array<string, true>
     */
    private array $files = [];

    /**
     * @var array{test: string, file: string, executed_lines: int, reported_lines: int}|null
     */
    private ?array $sample = null;

    public function __construct(
        private readonly string $mode,
        private readonly DriverCalls $driver,
        private readonly ?string $checkFile,
    )
    {
    }

    /**
     * @param list<string> $sourceFiles
     */
    public function prepare(array $sourceFiles): void
    {
        $this->sourceFiles = count($sourceFiles);
        $this->driver->prepare($sourceFiles);
    }

    public function start(): void
    {
        // PHPUnit does not emit the Finished event for a test that fails in its preparation.
        if ($this->running) {
            $this->lateStops++;
            $this->stop('');
        }

        $this->driver->start();
        $this->running = true;
    }

    public function stop(string $test): void
    {
        if (!$this->running) {
            return;
        }

        $lines = $this->driver->stop();
        $this->running = false;

        if ($this->checkFile !== null) {
            $this->count($test, $lines);
        }
    }

    public function writeCheckFile(): void
    {
        if ($this->checkFile === null) {
            return;
        }

        $check = [
            'mode' => $this->mode,
            'php_code_coverage' => Version::id(),
            'loaded' => [
                'turbocov' => extension_loaded('turbocov'),
                'pcov' => extension_loaded('pcov'),
                'xdebug' => extension_loaded('xdebug'),
            ],
            'xdebug_flags' => $this->driver instanceof XdebugCalls ? $this->driver->describeFlags() : null,
            'source_files' => $this->sourceFiles,
            'tests' => $this->tests,
            'tests_with_executed_lines' => $this->testsWithExecutedLines,
            'files_with_data' => count($this->files),
            'executed_lines_of_all_tests' => $this->executedLines,
            'late_stops' => $this->lateStops,
            'sample' => $this->sample,
            'turbocov_status' => $this->mode === 'turbocov' ? get_status() : null,
        ];

        file_put_contents($this->checkFile, json_encode($check, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES | JSON_THROW_ON_ERROR) . "\n");
    }

    /**
     * @param array<string, array<int, int>> $lines
     */
    private function count(string $test, array $lines): void
    {
        $this->tests++;
        $executedInTest = 0;

        foreach ($lines as $file => $fileLines) {
            $this->files[$file] = true;
            $executedInFile = count(array_keys($fileLines, 1, true));
            $executedInTest += $executedInFile;

            if ($this->sample === null && $executedInFile > 0) {
                $this->sample = [
                    'test' => $test,
                    'file' => $file,
                    'executed_lines' => $executedInFile,
                    'reported_lines' => count($fileLines),
                ];
            }
        }

        if ($executedInTest > 0) {
            $this->testsWithExecutedLines++;
            $this->executedLines += $executedInTest;
        }
    }

}
