--TEST--
Test suspend in function registered with register_shutdown_function after an uncaught exception
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

register_shutdown_function(function () use ($loop): void {
    $promise = new Success($loop);
    $promise->schedule(Fiber::this());
    Fiber::suspend($loop->getScheduler());
    echo "unreachable";
});

$promise = new Success($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop->getScheduler());

throw new Exception('test');

--EXPECTF--
Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d

Fatal error: Uncaught FiberError: Cannot suspend during shutdown in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(FiberScheduler))
#1 [internal function]: {closure}()
#2 {main}
  thrown in %s on line %d
