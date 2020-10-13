--TEST--
Resume fiber from wrong scheduler
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop1 = new Loop;
$loop2 = new Loop;

$promise = new Promise($loop1);
$loop1->defer(fn() => $promise->resolve());;

$loop2->defer(function () use ($loop1, $loop2): void {
    Fiber::run(function () use ($loop1): void {
        $promise = new Promise($loop1);
        $loop1->delay(30, fn() => $promise->resolve());;
        Fiber::await($promise, $loop1);
    });

    $loop2->delay(100, fn() => 0);
});


echo Fiber::await($promise, $loop2);

--EXPECTF--
Warning: Uncaught FiberError: Fiber resumed by a scheduler other than that provided to Fiber::await() in %s:%d
Stack trace:
#0 %s(%d): Fiber->continue(NULL, NULL)
#1 %s(%d): Success->{closure}()
#2 %s(%d): Loop->tick()
#3 (0): Loop->run()
#4 {main}
  thrown in %s on line %d

Fatal error: Uncaught FiberError thrown from FiberScheduler::run(): Fiber resumed by a scheduler other than that provided to Fiber::await() in %s on line %d
