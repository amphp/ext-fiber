--TEST--
Nested schedulers 3
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop1 = new Loop;
$loop2 = new Loop;

$loop1->defer(function () use ($loop1, $loop2): void {
    Fiber::run(function () use ($loop1, $loop2): void {
        $promise1 = new Promise($loop1);
        $promise2 = new Promise($loop2);
        $promise3 = new Promise($loop2);
        $promise4 = new Promise($loop1);

        $loop1->delay(20, fn() => $promise1->resolve(1));
        echo Fiber::await($promise1, $loop1);

        $loop2->delay(5, fn() => $promise2->resolve(2));
        echo Fiber::await($promise2, $loop2);

        $loop2->delay(100, fn() => $promise3->resolve(3));
        echo Fiber::await($promise3, $loop2);

        $loop1->delay(5, fn() => $promise4->resolve(4));
        echo Fiber::await($promise4, $loop1);
    });

    Fiber::run(function () use ($loop1, $loop2): void {
        $promise5 = new Promise($loop1);
        $promise6 = new Promise($loop2);
        $promise7 = new Promise($loop1);

        $loop1->delay(5, fn() => $promise5->resolve(5));
        echo Fiber::await($promise5, $loop1);

        $loop2->delay(30, fn() => $promise6->resolve(6));
        echo Fiber::await($promise6, $loop2);

        $loop1->delay(5, fn() => $promise7->resolve(7));
        echo Fiber::await($promise7, $loop1);
    });
});

Fiber::await(new Success($loop1), $loop1);

// Note that $loop2 blocks $loop1 until $promise6 is resolved, which is why the timers appear to finish out of order.

--EXPECT--
5612374
