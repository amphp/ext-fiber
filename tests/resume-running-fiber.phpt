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

?>
--EXPECTF--
Fatal error: Uncaught FiberError: Cannot resume a fiber that is not suspended in %sresume-running-fiber.php:%d
Stack trace:
#0 %sresume-running-fiber.php(%d): Fiber->resume()
#1 [internal function]: {closure}()
#2 %sresume-running-fiber.php(%d): Fiber->start()
#3 {main}
  thrown in %sresume-running-fiber.php on line %d
