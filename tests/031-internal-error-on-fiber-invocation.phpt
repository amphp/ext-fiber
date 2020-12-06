--TEST--
Internal error on fiber invocation
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(fn() => Fiber::create(fn (int $x) => null)->start());

$promise = new Success($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop);

--EXPECTF--
Fatal error: Uncaught ArgumentCountError: Too few arguments to function {closure}(), 0 passed and exactly 1 expected in %s:%d
Stack trace:
#0 [fiber function](0): {closure}()
#1 {main}

Next FiberExit: Uncaught ArgumentCountError thrown from Loop::run(): Too few arguments to function {closure}(), 0 passed and exactly 1 expected in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend(Object(Loop))
#1 {main}
  thrown in %s on line %d
