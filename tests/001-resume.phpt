--TEST--
Test resume
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

echo Fiber::suspend(function (Continuation $continuation) use ($loop): void {
    $loop->defer(fn() => $continuation->resume('test'));
}, $loop);

--EXPECT--
test
