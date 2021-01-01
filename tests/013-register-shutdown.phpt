--TEST--
Test suspend in function registered with register_shutdown_function
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

register_shutdown_function(function () use ($loop): void {
    $promise = new Success($loop, 'shutdown');
    $promise->schedule(Fiber::this());
    echo Fiber::suspend($loop->getScheduler());
});

$promise = new Success($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop->getScheduler());

--EXPECT--
shutdown
