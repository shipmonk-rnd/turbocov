# GitLab CI

These jobs use the official `php` Docker image. They install turbocov with PIE in the job.

## Full coverage

The job prints the coverage of the whole project and gives GitLab a Cobertura report.

```yaml
phpunit:coverage:
  image: php:8.5-cli
  before_script:
    - apt-get update -qq && apt-get install -y -qq --no-install-recommends git unzip
    - curl -fsSL -o /usr/local/bin/pie https://github.com/php/pie/releases/latest/download/pie.phar && chmod +x /usr/local/bin/pie
    - curl -fsSL https://getcomposer.org/installer | php -- --install-dir=/usr/local/bin --filename=composer
    - pie install shipmonk/turbocov
    - php --ri turbocov
    - composer install --no-interaction --no-progress
  script:
    - php -d turbocov.collect_paths=src vendor/bin/phpunit --colors=never --coverage-text --coverage-cobertura=cobertura.xml
  coverage: '/^\s*Lines:\s*\d+\.\d+\%/'
  artifacts:
    reports:
      coverage_report:
        coverage_format: cobertura
        path: cobertura.xml
```

- `php --ri turbocov` stops the job when PHP does not load the extension.
- `coverage:` reads the line coverage from the text report. GitLab shows it in the merge request.
- PIE prints that `libtoolize` is missing. The build works without it.

To skip the build in each job, install turbocov in your own image:

```dockerfile
FROM php:8.5-cli
COPY --from=ghcr.io/php/pie:bin /pie /usr/bin/pie
RUN pie install shipmonk/turbocov
```

## Coverage of the changed files only

In a merge request pipeline, collect only the files that the merge request changes:

```yaml
phpunit:coverage:mr:
  image: php:8.5-cli
  rules:
    - if: $CI_PIPELINE_SOURCE == "merge_request_event"
  variables:
    GIT_DEPTH: 0
  before_script:
    # the same as above
  script:
    - git diff --name-only --diff-filter=AMR "$CI_MERGE_REQUEST_DIFF_BASE_SHA"...HEAD -- '*.php' > changed.txt
    - php -d turbocov.collect_paths=@changed.txt vendor/bin/phpunit --colors=never --coverage-cobertura=cobertura.xml
  artifacts:
    reports:
      coverage_report:
        coverage_format: cobertura
        path: cobertura.xml
```

Do not use `coverage:` in this job, and do not check a threshold on its report.
PHPUnit adds each file of `<source>` that has no data as not covered.
So each file that the merge request did not change has 0 % in this report, and the total is too low.
The lines of the changed files are right, and these are the lines that GitLab shows in the diff.

## Older PHPUnit

For PHPUnit before 13.3, use the [PCOV mode](pcov-mode.md):

```yaml
  before_script:
    # ...
    - pie install shipmonk/turbocov --enable-turbocov-impersonate-pcov --skip-enable-extension
    - echo "extension=turbocov" > /usr/local/etc/php/conf.d/turbocov.ini
    - php --ri pcov
  script:
    - php -d pcov.directory=src vendor/bin/phpunit --colors=never --coverage-text --coverage-cobertura=cobertura.xml
```
