--TEST--
Start running fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = Fiber::create(function () use ($loop): void {
    Fiber::suspend(fn(Fiber $fiber) => $loop->delay(10, fn() => $fiber->resume()), $loop);
});

$loop->defer(fn() => $fiber->start());
$loop->defer(fn() => $fiber->start());

Fiber::suspend(fn(Fiber $fiber) => $loop->defer(fn() => $fiber->resume()), $loop);

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot start a fiber that has already been started in %s:%d
Stack trace:
#0 %s(%d): Fiber->start()
#1 %s(%d): {closure}()
#2 %s(%d): Loop->tick()
#3 [fiber function](0): Loop->run()
#4 {main}

Next FiberExit: Uncaught FiberError thrown from Loop::run(): Cannot start a fiber that has already been started in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(Closure), Object(Loop))
#1 {main}
  thrown in %s on line %d
