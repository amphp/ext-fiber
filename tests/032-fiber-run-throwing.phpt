--TEST--
Immediate throw from fiber
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function (): void {
    Fiber::run(function (): void {
        throw new Exception('test');
    });
});

Fiber::await(new Promise($loop), $loop);

--EXPECTF--
Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 (0): {closure}()
#1 {main}

Next FiberExit: Uncaught Exception thrown from Loop::run(): test in %s:%d
Stack trace:
#0 %s(%d): Fiber::await(Object(Promise), Object(Loop))
#1 {main}
  thrown in %s on line %d
