--TEST--
Test fatal error with pending scheduler events
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->delay(100, fn() => print "fail\n");

$promise = new Promise($loop);
$loop->defer(fn() => $promise->resolve());
$promise->schedule(Fiber::this());

echo Fiber::suspend($loop->getSchedulerFiber());

trigger_error("Test error", E_USER_ERROR);

--EXPECTF--
Fatal error: Test error in %s on line %d
