--TEST--
Test fatal error in scheduler
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$promise = new Promise($loop);
$loop->defer(fn() => trigger_error("Test error", E_USER_ERROR));
$loop->defer(fn() => print "fail\n");
$promise->schedule(Fiber::this());

echo Fiber::suspend($loop);

--EXPECTF--
Fatal error: Test error in %s on line %d
