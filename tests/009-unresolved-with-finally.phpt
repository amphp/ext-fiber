--TEST--
FiberScheduler returns before resuming the fiber with a finally block
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

try {
    $fiber = Fiber::this();
    echo Fiber::suspend((new Loop)->getScheduler());
} finally {
    echo 'finally';
}

--EXPECTF--
finally
Fatal error: Uncaught FiberError: Scheduler fiber terminated before resuming the suspended fiber in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(FiberScheduler))
#1 {main}
  thrown in %s on line %d
