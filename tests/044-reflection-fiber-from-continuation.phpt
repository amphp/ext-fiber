--TEST--
ReflectionFiber::fromContinuation()
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

Fiber::suspend(function (Continuation $continuation) use ($loop): void {
    $reflection = ReflectionFiber::fromContinuation($continuation);
    var_dump($reflection->getExecutingFile());
    var_dump($reflection->getExecutingLine());
    $loop->defer(fn() => $continuation->resume());
}, $loop);

--EXPECTF--
string(%d) "%s/tests/%s.php"
int(12)
