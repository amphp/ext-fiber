--TEST--
ReflectionFiber::getTrace()
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = Fiber::create(function (ReflectionFiber $scheduler) use (&$reflection, $loop): void {
    echo "2)\n", formatStacktrace($scheduler->getTrace()), "\n\n";

    Fiber::suspend(function (Continuation $continuation) use (&$reflection, $scheduler, $loop): void {
        $reflection = ReflectionFiber::fromContinuation($continuation);
        echo "3)\n", formatStacktrace($reflection->getTrace()), "\n\n";
        $loop->defer(fn() => $continuation->resume());
    }, $loop);

    Fiber::suspend(function (Continuation $continuation) use (&$reflection, $loop): void {
        $loop->delay(20, fn() => $continuation->resume());
    }, $loop);

    echo "6)\n", formatStacktrace($scheduler->getTrace()), "\n\n";

    echo "7)\n", formatStacktrace($reflection->getTrace()), "\n\n";
});

$reflection = ReflectionFiber::fromFiber($fiber);

$loop->defer(function () use ($fiber, $reflection, $loop): void {
    $scheduler = ReflectionFiber::fromFiberScheduler($loop);
    $fiber->run($scheduler);
    echo "4)\n", formatStacktrace($reflection->getTrace()), "\n\n";
});

Fiber::suspend(function (Continuation $continuation) use ($loop): void {
    $reflection = ReflectionFiber::fromContinuation($continuation);
    echo "1)\n", formatStacktrace($reflection->getTrace()), "\n\n";
    $loop->delay(10, fn() => $continuation->resume());
}, $loop);

echo "5)\n", formatStacktrace($reflection->getTrace()), "\n\n";

--EXPECTF--
1)
#0 %s:35 ReflectionFiber->getTrace()
#1 {closure}()
#2 %s:37 Fiber::suspend()

2)
#0 %s:29 Fiber->run()
#1 %s:%d {closure}()
#2 %s:%d Loop->tick()
#3 [fiber function]:0 Loop->run()

3)
#0 %s:12 ReflectionFiber->getTrace()
#1 {closure}()
#2 %s:14 Fiber::suspend()
#3 [fiber function]:0 {closure}()

4)
#0 %s:14 Fiber::suspend()
#1 [fiber function]:0 {closure}()

5)
#0 %s:18 Fiber::suspend()
#1 [fiber function]:0 {closure}()

6)
#0 %s:17 Continuation->resume()
#1 %s:%d {closure}()
#2 %s:%d Loop->tick()
#3 [fiber function]:0 Loop->run()

7)
#0 %s:22 ReflectionFiber->getTrace()
#1 [fiber function]:0 {closure}()