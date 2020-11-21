--TEST--
Suspend callback throws an exception
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

Fiber::suspend(function (Fiber $fiber): void {
    throw new Exception('test');
}, $loop);

--EXPECTF--
Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 [internal function]: {closure}(Object(Fiber))
#1 %s(%d): Fiber::suspend(Object(Closure), Object(Loop))
#2 {main}
  thrown in %s on line %d
