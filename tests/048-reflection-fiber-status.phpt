--TEST--
ReflectionFiber status methods
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = new Fiber(function () use ($loop): void {
    $fiber = Fiber::this();
    $loop->defer(fn() => $fiber->resume());
    Fiber::suspend($loop);
});

$reflection = new ReflectionFiber($fiber);

var_dump($reflection->isStarted());
var_dump($reflection->isRunning());
var_dump($reflection->isSuspended());
var_dump($reflection->isTerminated());

$loop->defer(fn() => $fiber->start());

$fiber = Fiber::this();

$reflection = new ReflectionFiber($fiber);
var_dump($reflection->isStarted());
var_dump($reflection->isSuspended());
var_dump($reflection->isRunning());
var_dump($reflection->isTerminated());

$loop->delay(10, fn() => $fiber->resume());
Fiber::suspend($loop);

var_dump($reflection->isStarted());
var_dump($reflection->isSuspended());
var_dump($reflection->isRunning());
var_dump($reflection->isTerminated());

$reflection = new ReflectionFiberScheduler($loop);

var_dump($reflection->isStarted());
var_dump($reflection->isSuspended());
var_dump($reflection->isRunning());
var_dump($reflection->isTerminated());

--EXPECT--
bool(false)
bool(false)
bool(false)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
bool(true)
bool(false)
bool(false)
