--TEST--
Continuation reuse
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

echo Fiber::suspend(function (Continuation $continuation) use ($loop): void {
    var_dump($continuation->isPending());
    $loop->defer(fn () => $continuation->resume());
    $loop->defer(fn () => var_dump($continuation->isPending()));
    $loop->defer(fn () => $continuation->resume());
}, $loop);

--EXPECTF--
bool(true)
bool(false)

Fatal error: Uncaught FiberError: Continuation may only be used once in %s:%d
Stack trace:
#0 %s(%d): Continuation->resume()
#1 %s(%d): {closure}()
#2 %s(%d): Loop->tick()
#3 [fiber function](0): Loop->run()
#4 {main}

Next FiberExit: Exception thrown from scheduler during shutdown in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
