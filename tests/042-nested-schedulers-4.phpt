--TEST--
Nested schedulers 4
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop1 = new Loop;

$fiber = new Fiber(function () use ($loop1) {
    $loop2 = new Loop;

    $fiber = new Fiber(function () use ($loop1, $loop2): void {
        $loop3 = new Loop;

        $fiber = new Fiber(function () use ($loop1, $loop2, $loop3): void {
            $fiber = Fiber::this();

            $loop2->defer(fn() => $fiber->resume(1));
            echo Fiber::suspend($loop2->getSchedulerFiber());

            $loop1->defer(fn() => $fiber->resume(2));
            echo Fiber::suspend($loop1->getSchedulerFiber());

            $loop3->defer(fn() => $fiber->resume(3));
            echo Fiber::suspend($loop3->getSchedulerFiber());

            $loop2->defer(fn() => $fiber->resume(4));
            echo Fiber::suspend($loop2->getSchedulerFiber());

            $loop1->defer(fn() => $fiber->resume(5));
            echo Fiber::suspend($loop1->getSchedulerFiber());

            $loop3->defer(fn() => $fiber->resume(6));
            echo Fiber::suspend($loop3->getSchedulerFiber());
        });

        $loop3->defer(fn() => $fiber->start());

        $fiber = Fiber::this();

        $loop3->defer(fn() => $fiber->resume(7));
        echo Fiber::suspend($loop3->getSchedulerFiber());

        $loop2->defer(fn() => $fiber->resume(8));
        echo Fiber::suspend($loop2->getSchedulerFiber());

        $loop1->defer(fn() => $fiber->resume(9));
        echo Fiber::suspend($loop1->getSchedulerFiber());

        $loop2->defer(fn() => $fiber->resume('a'));
        echo Fiber::suspend($loop2->getSchedulerFiber());

        $loop1->defer(fn() => $fiber->resume('b'));
        echo Fiber::suspend($loop1->getSchedulerFiber());
    });

    $loop2->defer(fn() => $fiber->start());

    $fiber = Fiber::this();

    $loop2->defer(fn() => $fiber->resume('c'));
    echo Fiber::suspend($loop2->getSchedulerFiber());

    $loop1->defer(fn() => $fiber->resume('d'));
    echo Fiber::suspend($loop1->getSchedulerFiber());
});

$loop1->defer(fn() => $fiber->start());

$fiber = Fiber::this();
$loop1->defer(fn() => $fiber->resume('e'));
echo Fiber::suspend($loop1->getSchedulerFiber());

--EXPECT--
7ce1d2389456ab