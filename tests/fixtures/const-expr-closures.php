<?php

#[Attribute(Attribute::TARGET_ALL)]
final class Transform
{
    public function __construct(public Closure $callback) {}
}

#[Transform(static function (): string {
    return 'class attribute';
})]
final class Modern
{
    public const USED = static function (string $value): string {
        return strtoupper($value);
    };

    public const NOT_USED = static function (string $value): string {
        return strtolower($value);
    };

    public const LIST = [
        'used' => static function (): string {
            return 'list used';
        },
        'not used' => static function (): string {
            return 'list not used';
        },
    ];

    #[Transform(static function (): string {
        return 'constant attribute';
    })]
    public const ATTRIBUTED = 1;

    public Closure $property = static function (int $value): int {
        return $value + 1;
    };

    public static Closure $staticProperty = static function (int $value): int {
        return $value + 2;
    };

    #[Transform(static function (): string {
        return 'property attribute';
    })]
    public int $attributed = 1;

    public static function format(
        float $value,
        Closure $formatter = static function (float $value): string {
            return sprintf('%.1f', $value);
        },
    ): string {
        return $formatter($value);
    }

    public static function notCalled(
        Closure $formatter = static function (float $value): string {
            return sprintf('%.2f', $value);
        },
    ): string {
        return $formatter(1.0);
    }

    #[Transform(static function (string $value): string {
        return strtoupper($value)
            . '!';
    })]
    public static function shout(string $value): string
    {
        $transform = new ReflectionMethod(self::class, 'shout')
            ->getAttributes(Transform::class)[0]
            ->newInstance();

        return ($transform->callback)($value);
    }

    public static function parameterAttribute(
        #[Transform(static function (): string {
            return 'parameter attribute';
        })]
        int $value,
    ): int {
        return $value;
    }

    public static function nested(
        Closure $outer = static function (): Closure {
            return static function (
                Closure $inner = static function (): string {
                    return 'inner';
                },
            ): string {
                return $inner();
            };
        },
    ): string {
        return $outer()();
    }
}

const GLOBAL_USED = static function (int $value): int {
    return $value * 2;
};

const GLOBAL_NOT_USED = static function (int $value): int {
    return $value * 3;
};

#[Transform(static function (): string {
    return 'global constant attribute';
})]
const GLOBAL_ATTRIBUTED = 1;

function free_function(
    Closure $default = static function (): string {
        return 'free function';
    },
): string {
    return $default();
}

echo Modern::format(1.26), "\n";
echo Modern::shout('loud'), "\n";
echo (Modern::USED)('used'), "\n";
echo (Modern::LIST['used'])(), "\n";
echo (new Modern())->property->__invoke(1), "\n";
echo (Modern::$staticProperty)(1), "\n";
echo Modern::nested(), "\n";
echo (GLOBAL_USED)(2), "\n";
