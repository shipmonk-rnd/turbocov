<?php declare(strict_types = 1);

// Copies a PHPUnit configuration file and registers the benchmark extension in the copy.
// Usage: php add-extension.php <phpunit.xml> <copy.xml>

[, $input, $output] = $argv;

$document = new DOMDocument();
$document->preserveWhiteSpace = false;
$document->formatOutput = true;

if (!$document->load($input)) {
    fwrite(STDERR, "Cannot read {$input}\n");
    exit(1);
}

$root = $document->documentElement;
$extensions = $root->getElementsByTagName('extensions')->item(0);

if ($extensions === null) {
    $extensions = $root->appendChild($document->createElement('extensions'));
}

$bootstrap = $document->createElement('bootstrap');
$bootstrap->setAttribute('class', 'Turbocov\Benchmark\DriverOnlyExtension');
$extensions->appendChild($bootstrap);

$document->save($output);
