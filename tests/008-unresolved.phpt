--TEST--
FiberScheduler returns before resuming the fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$fiber = Fiber::this();

echo Fiber::suspend((new Loop)->getSchedulerFiber());

--EXPECTF--
Fatal error: Uncaught FiberError: Scheduler fiber terminated before resuming the suspended fiber in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(SchedulerFiber))
#1 {main}
  thrown in %s on line %d
