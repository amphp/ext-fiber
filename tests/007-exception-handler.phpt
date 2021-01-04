--TEST--
FiberScheduler throwing from run() with an uncaught exception handler
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

\set_exception_handler(function (\Throwable $exception): void {
    echo "Caught Exception: ", $exception->getMessage(), PHP_EOL;
});

$loop = new Loop;

$loop->defer(function (): void {
    throw new Error('test');
});

$promise = new Promise($loop);
$promise->schedule(Fiber::this());

echo Fiber::suspend($loop->getScheduler());

--EXPECTF--
Caught Exception: Uncaught Error thrown from fiber scheduler: test
