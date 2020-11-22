--TEST--
Resume running fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

echo Fiber::suspend(function (Fiber $fiber) use ($loop): void {
    var_dump($fiber->isSuspended());
    $loop->defer(fn() => $fiber->resume());
    $loop->defer(fn() => var_dump($fiber->isSuspended()));
    $loop->defer(fn() => var_dump($fiber->isRunning()));
    $loop->defer(fn() => var_dump($fiber->isTerminated()));
    $loop->defer(fn() => $fiber->resume());
}, $loop);

--EXPECTF--
bool(true)
bool(false)
bool(true)
bool(false)

Fatal error: Uncaught FiberError: Cannot resume a fiber that is not suspended in %s:%d
Stack trace:
#0 %s(%d): Fiber->resume()
#1 %s(%d): {closure}()
#2 %s(%d): Loop->tick()
#3 [fiber function](0): Loop->run()
#4 {main}
  thrown in %s on line %d
