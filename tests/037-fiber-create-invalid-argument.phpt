--TEST--
Invalid argument to Fiber constructor does not leak
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

try {
    $fiber = new Fiber(1);
} catch (TypeError $exception) {
    echo $exception->getMessage();
}

--EXPECT--
Fiber::__construct(): Argument #1 ($callable) must be a valid callback, no array or string given
