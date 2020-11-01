--TEST--
Internal error on fiber invocation
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function (): void {
    Fiber::run(function (int $x) {});
});

Fiber::await(new Promise($loop), $loop);

--EXPECTF--
Fatal error: Uncaught ArgumentCountError: Too few arguments to function {closure}(), 0 passed and exactly 1 expected in %s:%d
Stack trace:
#0 (0): {closure}()
#1 {main}

Next FiberExit: Uncaught ArgumentCountError thrown from Loop::run(): Too few arguments to function {closure}(), 0 passed and exactly 1 expected in %s:%d
Stack trace:
#0 %s(%d): Fiber::await(Object(Promise), Object(Loop))
#1 {main}
  thrown in %s on line %d
