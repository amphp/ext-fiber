--TEST--
Await on an Awaitable where the given FiberScheduler returns before resolving with a finally block
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$promise = new Promise($loop);

try {
    echo Fiber::await($promise, $loop);
} finally {
    echo 'finally';
}

--EXPECTF--
finally
Fatal error: Uncaught FiberExit: Loop::run() returned before resuming the fiber in %s:%d
Stack trace:
#0 %s(%d): Fiber::await(Object(Promise), Object(Loop))
#1 {main}
  thrown in %s on line %d
