--TEST--
Resume non-running fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(fn() => null);

$fiber->resume();

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot resume a fiber that is not suspended in %s018-resume-non-running-fiber.php:%d
Stack trace:
#0 %s018-resume-non-running-fiber.php(%d): Fiber->resume()
#1 {main}
  thrown in %s018-resume-non-running-fiber.php on line %d
