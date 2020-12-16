<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

// Create three new fibers and run them in the FiberScheduler.
$fiber = new Fiber(function () use ($loop): void {
    $fiber = Fiber::this();
    $loop->delay(1500, fn() => $fiber->resume(1));
    $value = Fiber::suspend($loop);
    var_dump($value);
});
$loop->defer(fn() => $fiber->start());

$fiber = new Fiber(function () use ($loop): void {
    $fiber = Fiber::this();
    $loop->delay(1000, fn() => $fiber->resume(2));
    $value = Fiber::suspend($loop);
    var_dump($value);
});
$loop->defer(fn() => $fiber->start());

$fiber = new Fiber(function () use ($loop): void {
    $fiber = Fiber::this();
    $loop->delay(2000, fn() => $fiber->resume(3));
    $value = Fiber::suspend($loop);
    var_dump($value);
});
$loop->defer(fn() => $fiber->start());

// Suspend the main thread to enter the FiberScheduler.
$fiber = Fiber::this();
$loop->delay(500, fn() => $fiber->resume(4));
$value = Fiber::suspend($loop);
var_dump($value);