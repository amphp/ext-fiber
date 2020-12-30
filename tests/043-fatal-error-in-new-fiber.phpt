--TEST--
Fatal error in new fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = new Fiber(function () use ($loop): void {
    trigger_error("Fatal error in fiber", E_USER_ERROR);
});

$loop->defer(fn() => $fiber->start());

$promise = new Success($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop->getSchedulerFiber());

--EXPECTF--
Fatal error: Fatal error in fiber in %s on line %d
