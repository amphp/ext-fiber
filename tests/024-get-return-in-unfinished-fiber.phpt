--TEST--
Fiber::getReturn() in unfinished fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(fn() => Fiber::suspend(1));

var_dump($fiber->start());

$fiber->getReturn();

--EXPECTF--
int(1)

Fatal error: Uncaught FiberError: Cannot get fiber return value: The fiber has not returned in %s024-get-return-in-unfinished-fiber.php:%d
Stack trace:
#0 %s024-get-return-in-unfinished-fiber.php(%d): Fiber->getReturn()
#1 {main}
  thrown in %s024-get-return-in-unfinished-fiber.php on line %d
