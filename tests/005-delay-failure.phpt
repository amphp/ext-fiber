--TEST--
Await on unresolved Awaitable that fails
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$promise = new Promise($loop);

$timeout = 100;

$loop->delay($timeout, fn() => $promise->fail(new Exception('test')));

$start = $loop->now();

try {
    echo Fiber::await($promise, $loop);
    throw new Exception('Fiber::await() did not throw');
} catch (Exception $exception) {
    echo $exception->getMessage();
}

if ($loop->now() - $start < $timeout) {
    throw new Exception(sprintf('Test took less than %dms', $timeout));
}

--EXPECT--
test
