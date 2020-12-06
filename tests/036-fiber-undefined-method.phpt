--TEST--
Calling undefined method on root fiber object
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$fiber = Fiber::this();
$fiber->resolve();

--EXPECTF--
Fatal error: Uncaught Error: Call to undefined method Fiber::resolve() in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
