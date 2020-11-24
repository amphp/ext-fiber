--TEST--
Multiple schedulers with the first throwing on exit
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop1 = new Loop;
$loop2 = new Loop;

Fiber::suspend(fn(Fiber $fiber) => $loop1->defer(fn() => $fiber->resume()), $loop1);

Fiber::suspend(fn(Fiber $fiber) => $loop2->defer(fn() => $fiber->resume()), $loop2);

$loop1->defer(function (): void {
    throw new Exception('test');
});

$loop1->defer(fn() => print "Should not be executed\n");

--EXPECTF--
Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 %s(%d): {closure}()
#1 %s(%d): Loop->tick()
#2 [fiber function](0): Loop->run()
#3 {main}
  thrown in %s on line %d
