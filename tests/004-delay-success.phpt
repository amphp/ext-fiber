--TEST--
Await on unresolved Awaitable that resolves successfully
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$promise = new Promise($loop);

$timeout = 100;

$loop->delay($timeout, fn() => $promise->resolve('test'));

$start = $loop->now();

echo Fiber::suspend($promise, $loop);

if ($loop->now() - $start < $timeout) {
    throw new Exception(sprintf('Test took less than %dms', $timeout));
}

--EXPECT--
test
