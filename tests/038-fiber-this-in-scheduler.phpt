--TEST--
Calling Fiber::this() in scheduler
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(fn() => Fiber::this());

$promise = new Success($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop);

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot call Fiber::this() within a fiber scheduler in %s:%d
Stack trace:
#0 %s(%d): Fiber::this()
#1 %s(%d): {closure}()
#2 %s(%d): Loop->tick()
#3 [fiber function](0): Loop->run()
#4 {main}

Next FiberExit: Uncaught FiberError thrown from Loop::run(): Cannot call Fiber::this() within a fiber scheduler in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(Loop))
#1 {main}
  thrown in %s on line %d
