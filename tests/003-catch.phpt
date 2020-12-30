--TEST--
Catch exception from throw
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;
$fiber = Fiber::this();
$loop->defer(fn() => $fiber->throw(new Exception('test')));

try {
    echo Fiber::suspend($loop->getSchedulerFiber());
} catch (Exception $exception) {
    echo $exception->getMessage();
}

--EXPECT--
test
