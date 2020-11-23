--TEST--
Start and resume outside of scheduler
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = Fiber::create(function (Fiber $fiber) use ($loop): void {
    Fiber::suspend(fn() => null, $loop);
});

try {
    $fiber->start();
} catch (FiberError $exception) {
    echo $exception->getMessage(), "\n";
}

try {
    $fiber->resume();
} catch (FiberError $exception) {
    echo $exception->getMessage(), "\n";
}

try {
    $fiber->throw(new Exception);
} catch (FiberError $exception) {
    echo $exception->getMessage(), "\n";
}

--EXPECT--
New fibers can only be started within a scheduler
Fibers can only be resumed within a scheduler
Fibers can only be resumed within a scheduler
