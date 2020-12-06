--TEST--
Invalid argument to Fiber::create() does not leak
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

try {
    $fiber = Fiber::create(1);
} catch (TypeError $exception) {
    echo $exception->getMessage();
}

--EXPECT--
Fiber::create(): Argument #1 ($callable) must be a valid callback, no array or string given
