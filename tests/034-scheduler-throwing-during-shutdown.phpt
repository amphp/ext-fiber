--TEST--
Throw from scheduler during shutdown
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop1 = new Loop;
$loop2 = new Loop;

$loop2->delay(10, function (): void {
    throw new Exception('test');
});

$loop1->delay(10, function (): void {
    echo "should not be executed\n";
});

$loop1->delay(1, function (): void {
    echo "should not be executed\n";
    throw new Exception('test');
});

$promise = new Success($loop1);
$promise->schedule(Fiber::this());
Fiber::suspend($loop1->getSchedulerFiber());

$promise = new Success($loop2);
$promise->schedule(Fiber::this());
Fiber::suspend($loop2->getSchedulerFiber());

$loop1->defer(function (): void {
    echo "should not be executed\n";
});

echo "done\n";

--EXPECTF--
done

Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 %s(%d): {closure}()
#1 %s(%d): Loop->tick()
#2 %s(%d): Loop->run()
#3 [fiber function](0): Loop->{closure}()
#4 {main}
  thrown in %s on line %d
