<?php declare(strict_types=1);

function zone(string $code): int
{
    switch ($code) {
        case 'eu':
            return 1;
        case 'us':
            return 2;
        case 'uk':
            return 3;
        case 'jp':
            return 4;
        case 'au':
            return 5;
        default:
            return 0;
    }
}
