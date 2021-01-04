--TEST--
Suspend main without resume path
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = Fiber::this();

Fiber::suspend((new Loop)->getScheduler());

--EXPECTF--
Fatal error: Uncaught FiberError: Scheduler fiber terminated before resuming the suspended fiber in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(FiberScheduler))
#1 {main}
  thrown in %s on line %d
