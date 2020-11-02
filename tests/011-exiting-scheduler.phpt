--TEST--
Test UnwindExit from FiberScheduler calling exit is ignored
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
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

echo Fiber::suspend(new Promise($loop), $loop);

--EXPECT--
exit