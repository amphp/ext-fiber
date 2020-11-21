--TEST--
Suspend callback calling suspend
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

echo Fiber::suspend(function () use ($loop): void {
    Fiber::suspend(fn() => null, $loop);
}, $loop);

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot suspend in a fiber that is not running in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(Closure), Object(Loop))
#1 [internal function]: {closure}(Object(Fiber))
#2 %s(%d): Fiber::suspend(Object(Closure), Object(Loop))
#3 {main}
  thrown in %s on line %d
