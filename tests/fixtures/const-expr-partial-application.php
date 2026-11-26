<?php

function keep(int $number, Closure $closure, int $other = 0): Closure
{
    return $closure;
}

#[Attribute(Attribute::TARGET_ALL)]
final class Holder
{
    public function __construct(public Closure $closure) {}
}

final class Partial
{
    public const USED = keep(?, static function (): string {
        return 'class constant';
    });

    public const NOT_USED = keep(?, closure: static function (): string {
        return 'named argument';
    });

    public static function notCalled(
        Closure $default = keep(?, static function (): string {
            return 'parameter default';
        }, ...),
    ): void {
    }
}

const GLOBAL_NOT_USED = keep(?, static function (): string {
    return 'global constant';
});

#[Holder(keep(1, static function (): string {
    return 'attribute';
}, ?))]
function attributed(): void
{
}

echo (Partial::USED)(1)(), "\n";

$function = function (
    Closure $default = keep(?, static function (): int {
        return 1;
    }),
): int {
    return $default(1)();
};

echo $function(), "\n";
