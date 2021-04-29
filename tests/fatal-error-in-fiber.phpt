--TEST--
Fatal error in new fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(function (): void {
    trigger_error("Fatal error in fiber", E_USER_ERROR);
});

$fiber->start();

?>
--EXPECTF--
Fatal error: Fatal error in fiber in %sfatal-error-in-fiber.php on line %d
