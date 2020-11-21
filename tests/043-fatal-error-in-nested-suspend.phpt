--TEST--
Fatal error in nested suspend callback
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = Fiber::create(function () use ($loop): void {
    Fiber::suspend(fn() => trigger_error("Fatal error in suspend callback", E_USER_ERROR), $loop);
});

$loop->defer(fn() => $fiber->start());

Fiber::suspend(new Promise($loop), $loop);

--EXPECTF--
Fatal error: Fatal error in suspend callback in %s on line %d
