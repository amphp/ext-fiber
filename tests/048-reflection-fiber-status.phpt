--TEST--
ReflectionFiber status methods
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = Fiber::create(function () use ($loop): void {
    Fiber::suspend(function (Fiber $fiber) use ($loop): void {
        $loop->defer(fn() => $fiber->resume());
    }, $loop);
});

$reflection = ReflectionFiber::fromFiber($fiber);

var_dump($reflection->isRunning());
var_dump($reflection->isSuspended());
var_dump($reflection->isTerminated());

$loop->defer(fn() => $fiber->start());

Fiber::suspend(function (Fiber $fiber) use ($loop): void {
    $reflection = ReflectionFiber::fromFiber($fiber);
    var_dump($reflection->isSuspended());
    var_dump($reflection->isRunning());
    var_dump($reflection->isTerminated());
    $loop->delay(10, fn() => $fiber->resume());
}, $loop);

var_dump($reflection->isSuspended());
var_dump($reflection->isRunning());
var_dump($reflection->isTerminated());
var_dump($reflection->isFiberScheduler());

$reflection = ReflectionFiber::fromFiberScheduler($loop);

var_dump($reflection->isSuspended());
var_dump($reflection->isRunning());
var_dump($reflection->isTerminated());
var_dump($reflection->isFiberScheduler());

--EXPECT--
bool(false)
bool(false)
bool(false)
bool(true)
bool(false)
bool(false)
bool(false)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(false)
bool(true)