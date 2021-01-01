--TEST--
Resuming non-running fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = new Fiber(function () use ($loop): void {
    $fiber = Fiber::this();
    $loop->delay(10, fn() => $fiber->resume());
    Fiber::suspend($loop->getScheduler());
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

$fiber = Fiber::this();
$loop->defer(fn() => $fiber->resume());
Fiber::suspend($loop->getScheduler());

--EXPECT--
Cannot resume a fiber that is not suspended
Cannot resume a fiber that is not suspended
