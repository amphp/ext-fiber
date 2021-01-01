--TEST--
Resume fiber from wrong scheduler
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop1 = new Loop;
$loop2 = new Loop;

$promise = new Promise($loop1);
$loop1->defer(fn() => $promise->resolve());;

$loop2->defer(function () use ($loop1, $loop2): void {
    $fiber = new Fiber(function () use ($loop1): void {
        $promise = new Promise($loop1);
        $loop1->delay(30, fn() => $promise->resolve());;
        $promise->schedule(Fiber::this());
        Fiber::suspend($loop1->getScheduler());
    });

    $loop2->delay(100, fn() => 0);

    $fiber->start();
});

$promise->schedule(Fiber::this());

echo Fiber::suspend($loop2->getScheduler());

--EXPECTF--
Fatal error: Uncaught FiberExit: Fiber resumed by a scheduler other than that provided to Fiber::suspend() in %s:%d
Stack trace:
#0 %s(%d): Fiber->resume(NULL)
#1 %s(%d): Success->{closure}()
#2 %s(%d): Loop->tick()
#3 %s(%d): Loop->run()
#4 [fiber function](0): Loop->{closure}()
#5 {main}
  thrown in %s on line %d
