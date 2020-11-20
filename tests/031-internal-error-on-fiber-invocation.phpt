--TEST--
Internal error on fiber invocation
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(fn() => Fiber::create(function (int $x) {})->run());

Fiber::suspend(new Promise($loop), $loop);

--EXPECTF--
Fatal error: Uncaught ArgumentCountError: Too few arguments to function {closure}(), 0 passed and exactly 1 expected in %s:%d
Stack trace:
#0 [fiber function](0): {closure}()
#1 {main}

Next FiberExit: Uncaught ArgumentCountError thrown from Fiber::run(): Too few arguments to function {closure}(), 0 passed and exactly 1 expected in %s:%d
Stack trace:
#0 %s(%d): Fiber->run()
#1 %s(%d): {closure}()
#2 %s(%d): Loop->tick()
#3 [fiber function](0): Loop->run()
#4 {main}
  thrown in %s on line %d
