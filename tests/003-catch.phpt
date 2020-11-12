--TEST--
Catch exception from throw
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
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
