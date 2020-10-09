--TEST--
Catch exception from failed Awaitable
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$promise = new Failure($loop, new Exception('test'));

try {
    echo Fiber::await($promise, $loop);
} catch (Exception $exception) {
    echo $exception->getMessage();
}

--EXPECT--
test
