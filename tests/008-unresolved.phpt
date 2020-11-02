--TEST--
Await on an Awaitable where the given FiberScheduler returns before resolving
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

echo Fiber::suspend(new Promise($loop), $loop);

--EXPECTF--
Fatal error: Uncaught FiberExit: Loop::run() returned before resuming the fiber in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(Promise), Object(Loop))
#1 {main}
  thrown in %s on line %d
