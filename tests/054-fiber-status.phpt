--TEST--
Fiber status methods
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = new Fiber(function () use ($loop): void {
    Fiber::suspend($loop->getScheduler());
});

$loop->defer(fn() => $fiber->start());

$main = Fiber::this();

var_dump($main->isStarted());
var_dump($main->isRunning());
var_dump($main->isSuspended());
var_dump($main->isTerminated());

var_dump($fiber->isStarted());

$scheduler = $loop->getScheduler();

var_dump($scheduler->isStarted());

$loop->delay(10, fn() => $main->resume());
Fiber::suspend($scheduler);

var_dump($scheduler->isStarted());
var_dump($scheduler->isSuspended());
var_dump($scheduler->isTerminated());

var_dump($fiber->isStarted());
var_dump($fiber->isSuspended());
var_dump($fiber->isTerminated());

--EXPECT--
bool(true)
bool(true)
bool(false)
bool(false)
bool(false)
bool(false)
bool(true)
bool(true)
bool(false)
bool(true)
bool(true)
bool(false)