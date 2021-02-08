--TEST--
Resume terminated fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(fn() => null);

$fiber->start();

$fiber->resume();

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot resume a fiber that is not suspended in %s020-resume-terminated-fiber.php:%d
Stack trace:
#0 %s020-resume-terminated-fiber.php(%d): Fiber->resume()
#1 {main}
  thrown in %s020-resume-terminated-fiber.php on line %d
