--TEST--
Resuming non-running fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = Fiber::create(function (Fiber $fiber) use ($loop): void {
    Fiber::suspend(fn() => $loop->delay(10, fn() => $fiber->resume()), $loop);
});

$loop->defer(function () use ($fiber): void {
    try {
        $fiber->resume();
    } catch (FiberError $exception) {
        echo $exception->getMessage(), "\n";
    }
});

$loop->defer(function () use ($fiber): void {
    try {
        $fiber->throw(new Exception);
    } catch (FiberError $exception) {
        echo $exception->getMessage(), "\n";
    }
});

Fiber::suspend(fn(Fiber $fiber) => $loop->defer(fn() => $fiber->resume()), $loop);

--EXPECT--
Cannot resume a fiber that is not suspended
Cannot resume a fiber that is not suspended
