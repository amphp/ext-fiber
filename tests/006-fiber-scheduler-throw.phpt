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

echo Fiber::await($promise, $loop);

--EXPECTF--
Warning: Uncaught Exception: test in %s:%d
Stack trace:
#0 %s/Loop.php(%d): {closure}()
#1 %s/Loop.php(%d): Loop->tick()
#2 (0): Loop->run()
#3 {main}
  thrown in %s on line %d

Fatal error: Uncaught Exception thrown from FiberScheduler::run(): test in %s on line %d
