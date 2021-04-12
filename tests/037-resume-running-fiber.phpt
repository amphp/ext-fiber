--TEST--
Resume running fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(function (): void {
    $self = Fiber::this();
    $self->resume();
});

$fiber->start();

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot resume a fiber that is not suspended in %s037-resume-running-fiber.php:%d
Stack trace:
#0 %s037-resume-running-fiber.php(%d): Fiber->resume()
#1 [internal function]: {closure}()
#2 {main}
  thrown in %s037-resume-running-fiber.php on line %d
