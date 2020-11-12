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

echo Fiber::suspend(function (Continuation $continuation) use ($loop, $timeout): void {
    $loop->delay($timeout, fn() => $continuation->resume('test'));
}, $loop);

if ($loop->now() - $start < $timeout) {
    throw new Exception(sprintf('Test took less than %dms', $timeout));
}

--EXPECT--
test
