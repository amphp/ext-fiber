--TEST--
FiberScheduler throwing from run()
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function (): void {
    throw new Exception('test');
});

$promise = new Promise($loop);
$promise->schedule(Fiber::this());

echo Fiber::suspend($loop->getSchedulerFiber());

--EXPECTF--
Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 %s(%d): {closure}()
#1 %s(%d): Loop->tick()
#2 %s(%d): Loop->run()
#3 [fiber function](0): Loop->{closure}()
#4 {main}

Next FiberExit: Uncaught Exception thrown from scheduler fiber: test in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(SchedulerFiber))
#1 {main}
  thrown in %s on line %d
