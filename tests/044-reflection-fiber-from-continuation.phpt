--TEST--
ReflectionFiber::fromFiber()
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

Fiber::suspend(function (Fiber $fiber) use ($loop): void {
    $reflection = ReflectionFiber::fromFiber($fiber);
    var_dump($reflection->getExecutingFile());
    var_dump($reflection->getExecutingLine());
    $loop->defer(fn() => $fiber->resume());
}, $loop);

--EXPECTF--
string(%d) "%s%etests%e%s.php"
int(12)
