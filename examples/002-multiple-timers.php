<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

// Create three new fibers and run them in the FiberScheduler.
$fiber = Fiber::create(function () use ($loop): void {
    delay($loop, 1500);
    var_dump(1);
});
$loop->defer(fn() => $fiber->run());

$fiber = Fiber::create(function () use ($loop): void {
    delay($loop, 1000);
    var_dump(2);
});
$loop->defer(fn() => $fiber->run());

$fiber = Fiber::create(function () use ($loop): void {
    delay($loop, 2000);
    var_dump(3);
});
$loop->defer(fn() => $fiber->run());

// Suspend the main thread to enter the FiberScheduler.
delay($loop, 500);
var_dump(4);
