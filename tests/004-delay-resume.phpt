--TEST--
Test delayed resume
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$timeout = 100;

$start = $loop->now();

$fiber = Fiber::this();

$loop->delay($timeout, fn() => $fiber->resume('test'));

echo Fiber::suspend($loop->getScheduler());

if ($loop->now() - $start < $timeout) {
    throw new Exception(sprintf('Test took less than %dms', $timeout));
}

--EXPECT--
test
