--TEST--
Test await in function registered with register_shutdown_function
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

register_shutdown_function(fn() => print Fiber::suspend(new Success($loop, 2), $loop));

Fiber::suspend(new Success($loop), $loop);

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot suspend during shutdown in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(Success), Object(Loop))
#1 [internal function]: {closure}()
#2 {main}
  thrown in %s on line %d
