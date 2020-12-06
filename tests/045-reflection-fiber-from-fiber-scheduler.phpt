--TEST--
ReflectionFiberScheduler
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

try {
    var_dump(new ReflectionFiberScheduler($loop));
} catch (Throwable $exception) {
    echo $exception->getMessage(), "\n";
}

$promise = new Success($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop);

$reflection = new ReflectionFiberScheduler($loop);

var_dump($reflection->getExecutingFile());
var_dump($reflection->getExecutingLine());

var_dump($reflection->getFiberScheduler() === $loop);

--EXPECTF--
Loop has not been used to suspend a fiber
string(%d) "%s%escripts%eSuccess.php"
int(21)
bool(true)
