--TEST--
Await on resolved Awaitable
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$promise = new Success($loop, 'test');

echo Fiber::await($promise, $loop);

--EXPECT--
test
