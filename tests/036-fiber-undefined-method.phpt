--TEST--
Calling undefined method on root fiber object
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

\Fiber::suspend(function (\Fiber $fiber) use ($loop): void {
    $loop->defer(fn() => $fiber->resolve());
}, $loop);

--EXPECTF--
Fatal error: Uncaught Error: Call to undefined method Fiber::resolve() in %s:%d
Stack trace:
#0 %s(%d): {closure}()
#1 %s(%d): Loop->tick()
#2 (0): Loop->run()
#3 {main}

Next FiberExit: Uncaught Error thrown from Loop::run(): Call to undefined method Fiber::resolve() in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(Closure), Object(Loop))
#1 {main}
  thrown in %s on line %d
