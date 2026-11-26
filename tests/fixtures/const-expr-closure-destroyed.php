<?php

$function = function (
    Closure $default = static function (): int {
        return 1;
    },
): int {
    return $default();
};

return $function();
