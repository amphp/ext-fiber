--TEST--
Fiber no callback provided
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

new Fiber;

--EXPECTF--
Fatal error: Uncaught ArgumentCountError: Fiber::__construct() expects exactly 1 argument, 0 given in %s:%d
Stack trace:
#0 %s(%d): Fiber->__construct()
#1 {main}
  thrown in %s on line %d
