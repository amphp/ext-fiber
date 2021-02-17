--TEST--
Fiber::getReturn() from unstarted fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(fn() => Fiber::suspend(1));

$fiber->getReturn();

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot get fiber return value: The fiber has not been started in %s025-get-return-from-unstarted-fiber.php:%d
Stack trace:
#0 %s025-get-return-from-unstarted-fiber.php(%d): Fiber->getReturn()
#1 {main}
  thrown in %s025-get-return-from-unstarted-fiber.php on line %d
