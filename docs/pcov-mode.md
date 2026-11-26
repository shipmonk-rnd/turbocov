# PCOV mode for older PHPUnit

PHPUnit before 13.3 cannot load a custom coverage driver. It selects its PCOV driver when PHP has an extension with the name `pcov`.

turbocov has a build option that registers the extension with that name. PHPUnit then uses turbocov and needs no change. You do not need `shipmonk/turbocov-phpunit` or the `driver` attribute in `phpunit.xml`.

On PHP 8.3, use this mode with PHPUnit 12. The direct use of turbocov needs PHPUnit 13.3, which needs PHP 8.4 or later.

This mode is a temporary aid, and a later version can remove it.

## Install

1. Remove the real PCOV. PHP cannot load both at the same time.
2. Build the extension with [PIE](https://github.com/php/pie):

    ```sh
    pie install shipmonk/turbocov --enable-turbocov-impersonate-pcov --skip-enable-extension
    ```

3. Add `extension=turbocov` to your `php.ini`.
4. Run `php --ri pcov`. The output must contain `Registered as => pcov`.

## Why PIE must not enable the extension

After a build, PIE adds the ini line and then checks that PHP has an extension with the name `turbocov`. This build has the name `pcov`, so the check fails. The result depends on the PHP installation:

- On the official Docker image, the ini line stays and the extension works. PIE prints a warning that you can ignore.
- On the PHP of [setup-php](https://github.com/shivammathur/setup-php), PIE removes the ini line again. PHP loads nothing, and PHPUnit finds no coverage driver. PIE still ends with status 0.

With `--skip-enable-extension`, PIE only builds the extension. The steps above then give the same result on each installation.

## Run PHPUnit

Run PHPUnit the same way as with PCOV:

```sh
php -d pcov.directory=src vendor/bin/phpunit --coverage-html coverage
```

`pcov.directory` is required. turbocov does not guess `src` like PCOV does. Without the setting, the report is empty.

## GitHub Actions

setup-php installs PIE and adds the settings to `php.ini`:

```yaml
- uses: shivammathur/setup-php@v2
  with:
    php-version: '8.5'
    coverage: none
    tools: pie
    ini-values: extension=turbocov, pcov.directory=src
- run: |
    sudo pie install shipmonk/turbocov --enable-turbocov-impersonate-pcov --skip-enable-extension
    php --ri pcov
```

- `coverage: none` keeps Xdebug and the real PCOV out.
- PIE needs `sudo` to copy the extension into the extension directory of PHP.
- `php --ri pcov` stops the job when PHP does not load the extension.
- The `pie` command prints one warning that PHP cannot load `turbocov`. That is expected: the ini line is there before the build.
