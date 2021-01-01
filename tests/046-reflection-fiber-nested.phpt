--TEST--
ReflectionFiber in nested fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = new Fiber(function () use ($loop): void {
    $reflection = new ReflectionFiber(Fiber::this());
    var_dump($reflection->getExecutingFile());
    var_dump($reflection->getExecutingLine());
});

$loop->defer(fn() => $fiber->start());

$promise = new Success($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop->getScheduler());

--EXPECTF--
string(%d) "%s%etests%e%s.php"
int(10)
