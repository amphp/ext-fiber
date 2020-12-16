--TEST--
Nested schedulers 3
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop1 = new Loop;
$loop2 = new Loop;

$loop1->defer(function () use ($loop1, $loop2): void {
    $fiber = new Fiber(function () use ($loop1, $loop2): void {
        $promise1 = new Promise($loop1);
        $promise2 = new Promise($loop2);
        $promise3 = new Promise($loop2);
        $promise4 = new Promise($loop1);

        $fiber = Fiber::this();

        $loop1->delay(20, fn() => $promise1->resolve(1));
        $promise1->schedule($fiber);
        echo Fiber::suspend($loop1);

        $loop2->delay(5, fn() => $promise2->resolve(2));
        $promise2->schedule($fiber);
        echo Fiber::suspend($loop2);

        $loop2->delay(100, fn() => $promise3->resolve(3));
        $promise3->schedule($fiber);
        echo Fiber::suspend($loop2);

        $loop1->delay(5, fn() => $promise4->resolve(4));
        $promise4->schedule($fiber);
        echo Fiber::suspend($loop1);
    });

    $fiber->start();

    $fiber = new Fiber(function () use ($loop1, $loop2): void {
        $promise5 = new Promise($loop1);
        $promise6 = new Promise($loop2);
        $promise7 = new Promise($loop1);

        $fiber = Fiber::this();

        $loop1->delay(5, fn() => $promise5->resolve(5));
        $promise5->schedule($fiber);
        echo Fiber::suspend($loop1);

        $loop2->delay(30, fn() => $promise6->resolve(6));
        $promise6->schedule($fiber);
        echo Fiber::suspend($loop2);

        $loop1->delay(5, fn() => $promise7->resolve(7));
        $promise7->schedule($fiber);
        echo Fiber::suspend($loop1);
    });

    $fiber->start();
});


$promise = new Success($loop1);
$promise->schedule(Fiber::this());
Fiber::suspend($loop1);

// Note that $loop2 blocks $loop1 until $promise6 is resolved, which is why the timers appear to finish out of order.

--EXPECT--
5612374
