--TEST--
Nested schedulers 4
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop1 = new Loop;

$fiber = Fiber::create(function () use ($loop1) {
    $loop2 = new Loop;

    $fiber = Fiber::create(function () use ($loop1, $loop2): void {
        $loop3 = new Loop;

        $fiber = Fiber::create(function () use ($loop1, $loop2, $loop3): void {
            echo Fiber::suspend(fn(Fiber $fiber) => $loop2->defer(fn() => $fiber->resume(1)), $loop2);
            echo Fiber::suspend(fn(Fiber $fiber) => $loop1->defer(fn() => $fiber->resume(2)), $loop1);
            echo Fiber::suspend(fn(Fiber $fiber) => $loop3->defer(fn() => $fiber->resume(3)), $loop3);
            echo Fiber::suspend(fn(Fiber $fiber) => $loop2->defer(fn() => $fiber->resume(4)), $loop2);
            echo Fiber::suspend(fn(Fiber $fiber) => $loop1->defer(fn() => $fiber->resume(5)), $loop1);
            echo Fiber::suspend(fn(Fiber $fiber) => $loop3->defer(fn() => $fiber->resume(6)), $loop3);
        });

        $loop3->defer(fn() => $fiber->start());

        echo Fiber::suspend(fn(Fiber $fiber) => $loop3->defer(fn() => $fiber->resume(7)), $loop3);
        echo Fiber::suspend(fn(Fiber $fiber) => $loop2->defer(fn() => $fiber->resume(8)), $loop2);
        echo Fiber::suspend(fn(Fiber $fiber) => $loop1->defer(fn() => $fiber->resume(9)), $loop1);
        echo Fiber::suspend(fn(Fiber $fiber) => $loop2->defer(fn() => $fiber->resume('a')), $loop2);
        echo Fiber::suspend(fn(Fiber $fiber) => $loop1->defer(fn() => $fiber->resume('b')), $loop1);
    });

    $loop2->defer(fn() => $fiber->start());

    echo Fiber::suspend(fn(Fiber $fiber) => $loop2->defer(fn() => $fiber->resume('c')), $loop2);
    echo Fiber::suspend(fn(Fiber $fiber) => $loop1->defer(fn() => $fiber->resume('d')), $loop1);
});

$loop1->defer(fn() => $fiber->start());

echo Fiber::suspend(fn(Fiber $fiber) => $loop1->defer(fn() => $fiber->resume('e')), $loop1);

--EXPECT--
7ce1d2389456ab