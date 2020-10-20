--TEST--
Test fatal error in scheduler
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$promise = new Promise($loop);
$loop->defer(fn() => trigger_error("Test error", E_USER_ERROR));
$loop->defer(fn() => print "fail\n");

echo Fiber::await($promise, $loop);

--EXPECTF--
Fatal error: Test error in %s on line %d
