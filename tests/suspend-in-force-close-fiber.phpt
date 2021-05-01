--TEST--
Suspend in force-closed fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(function (): void {
    try {
        Fiber::suspend();
    } finally {
        Fiber::suspend();
    }
});

$fiber->start();

unset($fiber);

?>
--EXPECTF--
Fatal error: Uncaught FiberExit: Fiber destroyed in %ssuspend-in-force-close-fiber.php:%d
Stack trace:
#0 %ssuspend-in-force-close-fiber.php(%d): Fiber::suspend()
#1 [internal function]: {closure}()
#2 {main}

Next FiberError: Cannot suspend in a force-closed fiber in %ssuspend-in-force-close-fiber.php:%d
Stack trace:
#0 %ssuspend-in-force-close-fiber.php(%d): Fiber::suspend()
#1 [internal function]: {closure}()
#2 {main}
  thrown in %ssuspend-in-force-close-fiber.php on line %d
