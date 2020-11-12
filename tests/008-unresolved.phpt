--TEST--
FiberScheduler returns before resuming the fiber
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

echo Fiber::suspend(function (Continuation $continuation): void {
    // Empty callback.
}, new Loop);

--EXPECTF--
Fatal error: Uncaught FiberExit: Loop::run() returned before resuming the fiber in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(Closure), Object(Loop))
#1 {main}
  thrown in %s on line %d
