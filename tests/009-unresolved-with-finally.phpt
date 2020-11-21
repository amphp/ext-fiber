--TEST--
FiberScheduler returns before resuming the fiber with a finally block
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

try {
    echo Fiber::suspend(function (Fiber $fiber): void {
        // Empty callback.
    }, new Loop);
} finally {
    echo 'finally';
}

--EXPECTF--
finally
Fatal error: Uncaught FiberExit: Loop::run() returned before resuming the fiber in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(Closure), Object(Loop))
#1 {main}
  thrown in %s on line %d
