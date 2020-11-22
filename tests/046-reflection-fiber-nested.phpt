--TEST--
ReflectionFiber::fromFiber() in nested fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = Fiber::create(function () use ($loop): void {
    Fiber::suspend(function (Fiber $fiber) use ($loop): void {
        $reflection = ReflectionFiber::fromFiber($fiber);
        var_dump($reflection->getExecutingFile());
        var_dump($reflection->getExecutingLine());
        $loop->defer(fn() => $fiber->resume());
    }, $loop);
});

$loop->defer(fn() => $fiber->start());

Fiber::suspend(new Success($loop), $loop);

--EXPECTF--
string(%d) "%s%etests%e%s.php"
int(13)
