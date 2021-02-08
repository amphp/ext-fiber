--TEST--
Test throwing into fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(function (): void {
    Fiber::suspend('test');
    throw new Exception('test');
});

$value = $fiber->start();
var_dump($value);

$fiber->resume($value);

--EXPECTF--
string(4) "test"

Fatal error: Uncaught Exception: test in %s005-failing-fiber.php:%d
Stack trace:
#0 [fiber function](0): {closure}()
#1 {main}
  thrown in %s005-failing-fiber.php on line %d
