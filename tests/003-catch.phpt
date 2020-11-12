--TEST--
Catch exception from throw
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

try {
    echo Fiber::suspend(function (Continuation $continuation) use ($loop): void {
        $loop->defer(fn() => $continuation->throw(new Exception('test')));
    }, $loop);
} catch (Exception $exception) {
    echo $exception->getMessage();
}

--EXPECT--
test
