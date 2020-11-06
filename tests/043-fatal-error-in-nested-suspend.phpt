--TEST--
Fatal error in nested suspend callback
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function () use ($loop): void {
    Fiber::run(function () use ($loop): void {
        Fiber::suspend(fn() => trigger_error("Fatal error in suspend callback", E_USER_ERROR), $loop);
    });
});

Fiber::suspend(new Promise($loop), $loop);

--EXPECTF--
Fatal error: Fatal error in suspend callback in %s on line %d
