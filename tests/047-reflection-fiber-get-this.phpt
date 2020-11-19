--TEST--
ReflectionFiber::getThis()
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

class FiberTest
{
    function suspend(Loop $loop): void
    {
        Fiber::suspend(function (Continuation $continuation) use ($loop): void {
            $reflection = ReflectionFiber::fromContinuation($continuation);
            echo get_class($reflection->getThis());
            $loop->defer(fn() => $continuation->resume());
        }, $loop);
    }
}

$loop = new Loop;

$loop->defer(function () use ($loop): void {
    Fiber::run(function () use ($loop): void {
        $test = new FiberTest;
        $test->suspend($loop);
    });
});

Fiber::suspend(new Success($loop), $loop);

--EXPECT--
FiberTest
