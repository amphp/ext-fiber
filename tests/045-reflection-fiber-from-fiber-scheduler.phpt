--TEST--
ReflectionFiberScheduler
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

// Get reflection before using scheduler.
$reflection = new ReflectionFiberScheduler($loop);

try {
    var_dump($reflection->getExecutingLine());
} catch (Throwable $exception) {
    echo $exception->getMessage(), "\n";
}

$promise = new Success($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop);

// Reflection should be valid after using scheduler.
var_dump($reflection->getExecutingFile());
var_dump($reflection->getExecutingLine());

var_dump($reflection->getScheduler() === $loop);

--EXPECTF--
Cannot fetch information from a fiber that has not been started or is terminated
string(%d) "%s%escripts%eSuccess.php"
int(21)
bool(true)
