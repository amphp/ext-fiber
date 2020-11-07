--TEST--
Throw from scheduler during shutdown
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop1 = new Loop;
$loop2 = new Loop;

$loop1->delay(10, function (): void {
    throw new Exception('test');
});

$loop1->delay(10, function (): void {
    echo "should not be executed\n";
});

$loop2->delay(1, function (): void {
    echo "should not be executed\n";
    throw new Exception('test');
});

Fiber::suspend(new Success($loop1), $loop1);

Fiber::suspend(new Success($loop2), $loop2);

$loop2->defer(function (): void {
    echo "should not be executed\n";
});

echo "done\n";

--EXPECTF--
done

Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 %s(%d): {closure}()
#1 %s(%d): Loop->tick()
#2 [fiber function](0): Loop->run()
#3 {main}

Next FiberExit: Exception thrown from scheduler during shutdown in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
