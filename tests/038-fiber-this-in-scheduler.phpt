--TEST--
Calling Fiber::this() in scheduler
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(fn() => Fiber::this());

$promise = new Success($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop->getSchedulerFiber());

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot call Fiber::this() within a fiber scheduler in %s:%d
Stack trace:
#0 %s(%d): Fiber::this()
#1 %s(%d): {closure}()
#2 %s(%d): Loop->tick()
#3 %s(%d): Loop->run()
#4 [fiber function](0): Loop->{closure}()
#5 {main}

Next FiberExit: Uncaught FiberError thrown from scheduler fiber: Cannot call Fiber::this() within a fiber scheduler in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(SchedulerFiber))
#1 {main}
  thrown in %s on line %d
