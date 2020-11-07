--TEST--
FiberScheduler throwing from run()
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function (): void {
    throw new Exception('test');
});

$promise = new Promise($loop);

echo Fiber::suspend($promise, $loop);

--EXPECTF--
Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 %s(%d): {closure}()
#1 %s(%d): Loop->tick()
#2 [fiber function](0): Loop->run()
#3 {main}

Next FiberExit: Uncaught Exception thrown from Loop::run(): test in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(Promise), Object(Loop))
#1 {main}
  thrown in %s on line %d
