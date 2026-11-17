<?php declare(strict_types = 1);

namespace Turbocov\Benchmark;

use LogicException;
use SebastianBergmann\CodeCoverage\Version;
use function in_array;
use function version_compare;
use function xdebug_get_code_coverage;
use function xdebug_info;
use function xdebug_set_filter;
use function xdebug_start_code_coverage;
use function xdebug_stop_code_coverage;
use const XDEBUG_CC_DEAD_CODE;
use const XDEBUG_CC_UNUSED;
use const XDEBUG_FILTER_CODE_COVERAGE;
use const XDEBUG_PATH_INCLUDE;

/**
 * The calls of SebastianBergmann\CodeCoverage\Driver\XdebugDriver for line coverage.
 */
final class XdebugCalls implements DriverCalls
{

    private int $flags = XDEBUG_CC_UNUSED;

    public function prepare(array $sourceFiles): void
    {
        if (!in_array('coverage', xdebug_info('mode'), true)) {
            throw new LogicException('xdebug.mode does not contain coverage');
        }

        // php-code-coverage before 14.3 detects dead code for line coverage. Later versions do not.
        if (version_compare(Version::id(), '14.3', '<')) {
            $this->flags |= XDEBUG_CC_DEAD_CODE;
        }

        if ($sourceFiles !== []) {
            xdebug_set_filter(XDEBUG_FILTER_CODE_COVERAGE, XDEBUG_PATH_INCLUDE, $sourceFiles);
        }
    }

    public function start(): void
    {
        xdebug_start_code_coverage($this->flags);
    }

    public function stop(): array
    {
        $data = xdebug_get_code_coverage();

        xdebug_stop_code_coverage();

        return $data;
    }

    public function describeFlags(): string
    {
        return ($this->flags & XDEBUG_CC_DEAD_CODE) !== 0 ? 'XDEBUG_CC_UNUSED | XDEBUG_CC_DEAD_CODE' : 'XDEBUG_CC_UNUSED';
    }

}
