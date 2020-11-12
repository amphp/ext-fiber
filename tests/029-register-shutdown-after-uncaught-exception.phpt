--TEST--
Test suspend in function registered with register_shutdown_function after an uncaught exception
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

register_shutdown_function(function () use ($loop): void {
    Fiber::suspend(new Success($loop), $loop);
    echo "unreachable";
});

Fiber::suspend(new Success($loop), $loop);

throw new Exception('test');

--EXPECTF--
Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d

Fatal error: Uncaught FiberError: Cannot suspend during shutdown in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(Success), Object(Loop))
#1 [internal function]: {closure}()
#2 {main}
  thrown in %s on line %d
