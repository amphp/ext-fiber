--TEST--
ReflectionFiber invalid arguments
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

new ReflectionFiber;

--EXPECTF--
Fatal error: Uncaught ArgumentCountError: ReflectionFiber::__construct() expects exactly 1 argument, 0 given in %s:%d
Stack trace:
#0 %s(%d): ReflectionFiber->__construct()
#1 {main}
  thrown in %s on line %d
