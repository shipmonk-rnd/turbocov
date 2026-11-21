<?php declare(strict_types=1);

function join_words(string $first, string $second, string $third): string
{
    return "$first $second $third";
}

$greet = join_words('good', ?, 'world');
$never_called = join_words(?, ?, 'unused');
$with_name = join_words(
    third: 'c',
    first: 'a',
    second: ?,
);
$upper = array_map(
    strtoupper(...),
    ['x', 'y'],
);

echo $greet('morning'), "\n";
echo $greet('evening'), "\n";
echo $with_name('b'), "\n";
echo implode(',', $upper), "\n";
