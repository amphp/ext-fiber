--TEST--
Test UnwindExit from FiberScheduler calling exit is ignored
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

set_exception_handler(function (Throwable $exception): void {
    throw $exception; // Should not be called.
});

$loop->defer(function (): void {
    echo 'exit';
    exit;
});

$promise = new Promise($loop);
$promise->schedule(Fiber::this());

echo Fiber::suspend($loop->getSchedulerFiber());

--EXPECT--
exit