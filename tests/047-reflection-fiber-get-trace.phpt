--TEST--
ReflectionFiber::getTrace()
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = Fiber::create(function (ReflectionFiber $scheduler) use ($loop): void {
    echo "2)\n", formatStacktrace($scheduler->getTrace()), "\n\n";

    $fiber = Fiber::this();

    $reflection = new ReflectionFiber($fiber);
    echo "3)\n", formatStacktrace($reflection->getTrace()), "\n\n";

    $loop->defer(fn() => $fiber->resume());
    Fiber::suspend($loop);

    $loop->delay(20, fn() => $fiber->resume());

    Fiber::suspend($loop);

    echo "6)\n", formatStacktrace($scheduler->getTrace()), "\n\n";

    echo "7)\n", formatStacktrace($reflection->getTrace()), "\n\n";
});

$reflection = new ReflectionFiber($fiber);

$loop->defer(function () use ($fiber, $reflection, $loop): void {
    $scheduler = new ReflectionFiberScheduler($loop);
    $fiber->start($scheduler);
    echo "4)\n", formatStacktrace($reflection->getTrace()), "\n\n";
});

$fiber = Fiber::this();

$reflection = new ReflectionFiber($fiber);
echo "1)\n", formatStacktrace($reflection->getTrace()), "\n\n";
$loop->delay(10, fn() => $fiber->resume());

Fiber::suspend($loop);

echo "5)\n", formatStacktrace($reflection->getTrace()), "\n\n";

--EXPECTF--
1)
#0 %s:38 ReflectionFiber->getTrace()

2)
#0 %s:31 Fiber->start()
#1 %s:%d {closure}()
#2 %s:%d Loop->tick()
#3 [fiber function]:0 Loop->run()

3)
#0 %s:13 ReflectionFiber->getTrace()
#1 [fiber function]:0 {closure}()

4)
#0 %s:16 Fiber::suspend()
#1 [fiber function]:0 {closure}()

5)
#0 %s:43 ReflectionFiber->getTrace()

6)
#0 %s:18 Fiber->resume()
#1 %s:%d {closure}()
#2 %s:%d Loop->tick()
#3 [fiber function]:0 Loop->run()

7)
#0 %s:24 ReflectionFiber->getTrace()
#1 [fiber function]:0 {closure}()