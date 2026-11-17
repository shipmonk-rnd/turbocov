# Driver-only benchmark

This benchmark measures the coverage driver alone. It compares turbocov, PCOV and Xdebug. The chart in the [main README](../README.md#performance) shows its result for Carbon.

A test run with a coverage report also has the work of the coverage library of PHPUnit. That work is the same with each driver, and this benchmark does not have it.

## What it measures

A PHPUnit extension starts the driver before each test. It stops the driver after the test and reads the line data. Then it drops the data.

- The coverage of PHPUnit is off. The coverage library does no work.
- Each mode makes the same calls as the driver class that PHPUnit uses.
- The baseline loads the same PHPUnit extension, so the cost of the events is equal in all modes.
- Each process loads one coverage extension. OPcache is off.
- Each driver gets the full source directory of the project as its path limit.

| Mode | Calls | Path limit |
|---|---|---|
| `none` | No calls | |
| `turbocov` | As `TurbocovDriver` in [shipmonk/turbocov-phpunit](https://github.com/shipmonk-rnd/turbocov-phpunit): `start()`, `stop()`, and `get_executable_lines()` for new files | `turbocov.collect_paths` |
| `pcov` | As `PcovDriver` in phpunit/php-code-coverage: `start()`, `stop()`, `waiting()`, `collect()`, `clear()` | `pcov.directory` |
| `xdebug` | As `XdebugDriver` in phpunit/php-code-coverage: `xdebug_set_filter()` one time, then `xdebug_start_code_coverage()`, `xdebug_get_code_coverage()`, `xdebug_stop_code_coverage()` | The filter of Xdebug |

The flags of Xdebug are the flags of the installed version of phpunit/php-code-coverage. Before version 14.3 the library uses `XDEBUG_CC_UNUSED | XDEBUG_CC_DEAD_CODE` for line coverage. Version 14.3 and later use only `XDEBUG_CC_UNUSED`.

The time of a run is the time that PHPUnit prints. The added time of a mode is its time minus the time of `none` in the same round.

## What it does not measure

- The work of the coverage library: filters, the merge of the data of each test, the reports. On dead-code-detector that work is about 0.8 s for each run, more than the time that turbocov adds.
- The conversion of the line data to objects of the coverage library.
- Memory.
- OPcache on.
- Tests that run in a separate process. The extension does not run in the child process.

## How to run it

You need Docker and git. Each container has a memory limit of 6 GB.

```sh
benchmark/run.sh        # 8 rounds
benchmark/run.sh 20     # 20 rounds
BENCHMARK_SUITES="carbon commonmark" benchmark/run.sh
```

The script does these steps:

1. It builds the image `turbocov-benchmark`: PHP 8.5, PCOV and Xdebug from PECL, turbocov from this checkout.
2. It fetches each suite at a fixed commit or tag into `benchmark/work/` and installs its dependencies.
3. It runs each mode one time with a check. The check fails when a driver gives no line data, or when the baseline has a driver.
4. It runs the rounds. In each round the modes run one after the other: `none`, `turbocov`, `pcov`, `xdebug`.
5. It prints the versions and the statistics of each suite.

The raw times are in `benchmark/work/times-<date>.tsv`. The output of each run is in `benchmark/work/logs/`. Set `BENCHMARK_WORK_DIR` to use a different directory.

A full run with 8 rounds takes about 65 minutes. Most of that time is Xdebug on Carbon. Do not use the machine for other work during the run.

## Suites

| Suite | Version | Tests | Source files | Profile |
|---|---|---|---|---|
| [shipmonk-rnd/dead-code-detector](https://github.com/shipmonk-rnd/dead-code-detector) | commit `ae624a6` | 870 | 68 | Static analysis. Few tests, each with much work. PHPUnit 10.5. |
| [briannesbitt/Carbon](https://github.com/briannesbitt/Carbon) | commit `c997755` | 6,349 | 918 | Date library. Many small tests, many source files. PHPUnit 10.5. |
| [thephpleague/commonmark](https://github.com/thephpleague/commonmark) | tag 2.10.7 | 4,447 | 302 | Markdown parser. Many small tests, a short suite. PHPUnit 13. |

The suites run as they are in their repositories. The script only adds the extension to a copy of the PHPUnit configuration.

## Where the driver starts and stops

PHPUnit starts its coverage directly before `TestCase::runBare()` and stops it directly after. The extension uses the nearest events:

- Start: `Test\PreparationStarted`. It is the first action of `runBare()`.
- Stop: `Test\Finished`. PHPUnit emits it after its checks for risky tests. Thus the driver is active for a small amount of PHPUnit code more than in a real run.
- A test that fails in its preparation has no `Finished` event. The driver then stops when the next test starts. The check run counts these tests.

PHPUnit creates its driver after it loaded the test classes. The extension does the same at `TestRunner\ExecutionStarted`. This is important for Xdebug: its filter applies only to files that PHP compiles after `xdebug_set_filter()`. Files that PHP compiled earlier stay in the data of Xdebug, in a real run too.

## Limits

- The numbers are from one machine. Other hardware and other PHP builds give other numbers.
- Runs have noise. The time that turbocov adds is small, and it can be smaller than the spread between runs. The ratio to turbocov is then not exact. Read the minimum and the maximum, not only the mean.
- [results-2026-11-17.md](results-2026-11-17.md) has the result of one run.
- Three suites are not all PHP projects. A suite that waits for a database shows almost no difference between the drivers.
- The drivers do not have the same features. Xdebug can also collect branch coverage and path coverage and is a debugger. PCOV and turbocov collect only line coverage. turbocov has the limits that the main README lists.
- Carbon and commonmark have no lock file. Their dependencies, also PHPUnit, can be newer in a later run. The script prints the versions.
- The image `php:8.5-cli` gets new patch versions of PHP.
