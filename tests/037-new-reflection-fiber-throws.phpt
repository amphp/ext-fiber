--TEST--
Cannot construct ReflectionFiber object with new
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

new ReflectionFiber;

--EXPECTF--
Fatal error: Uncaught Error: Use the static constructors to create an instance of "ReflectionFiber" in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
