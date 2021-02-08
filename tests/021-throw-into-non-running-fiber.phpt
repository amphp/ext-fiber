--TEST--
Throw into non-running fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(fn() => null);

$fiber->throw(new Exception('test'));

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot resume a fiber that is not suspended in %s021-throw-into-non-running-fiber.php:%d
Stack trace:
#0 %s021-throw-into-non-running-fiber.php(%d): Fiber->throw(Object(Exception))
#1 {main}
  thrown in %s021-throw-into-non-running-fiber.php on line %d
