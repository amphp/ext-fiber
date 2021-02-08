--TEST--
Start on already running fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(function (): void {
    Fiber::suspend();
});

$fiber->start();

$fiber->start();

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot start a fiber that has already been started in %s019-double-start.php:%d
Stack trace:
#0 %s019-double-start.php(%d): Fiber->start()
#1 {main}
  thrown in %s019-double-start.php on line %d
