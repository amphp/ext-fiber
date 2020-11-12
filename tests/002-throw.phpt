--TEST--
Test throw
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

echo Fiber::suspend(function (Continuation $continuation) use ($loop): void {
    $loop->defer(fn() => $continuation->throw(new Exception('test')));
}, $loop);

--EXPECTF--
Fatal error: Uncaught Exception: test in %s.php:%d
Stack trace:
#0 %s(%d): {closure}()
#1 %s(%d): Loop->tick()
#2 [fiber function](0): Loop->run()
#3 {main}
  thrown in %s on line %d
