--TEST--
Fiber::getReturn() after a fiber throws
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(fn() => throw new Exception('test'));

try {
    $fiber->start();
} catch (Exception $exception) {
    echo $exception->getMessage(), "\n";
}

$fiber->getReturn();

--EXPECTF--
test

Fatal error: Uncaught FiberError: Cannot get fiber return value: The fiber threw an exception in %s023-get-return-after-throwing.php:%d
Stack trace:
#0 %s023-get-return-after-throwing.php(%d): Fiber->getReturn()
#1 {main}
  thrown in %s023-get-return-after-throwing.php on line %d
