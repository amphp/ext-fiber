--TEST--
Immediate throw from fiber after suspend
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = new Fiber(function () use ($loop): void {
    $promise = new Success($loop);
    $promise->schedule(Fiber::this());
    Fiber::suspend($loop->getSchedulerFiber());
    throw new Exception('test');
});

$loop->defer(fn() => $fiber->start());

$promise = new Promise($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop->getSchedulerFiber());

--EXPECTF--
Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 [fiber function](0): {closure}()
#1 {main}

Next FiberExit: Uncaught Exception thrown from scheduler fiber: test in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(SchedulerFiber))
#1 {main}
  thrown in %s on line %d
