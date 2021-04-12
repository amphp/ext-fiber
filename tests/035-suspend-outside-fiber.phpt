--TEST--
Suspend outside fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$value = Fiber::suspend(1);

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot suspend outside of a fiber in %s035-suspend-outside-fiber.php:%d
Stack trace:
#0 %s035-suspend-outside-fiber.php(%d): Fiber::suspend(1)
#1 {main}
  thrown in %s035-suspend-outside-fiber.php on line %d
