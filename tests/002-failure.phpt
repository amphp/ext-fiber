--TEST--
Await on failed Awaitable
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$promise = new Failure($loop, new Exception('test'));

echo Fiber::await($promise, $loop);

--EXPECTF--
Fatal error: Uncaught Exception: test in %s.php:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
