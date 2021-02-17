--TEST--
Fatal error in a fiber with other active fibers
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber1 = new Fiber(fn() => Fiber::suspend(1));

$fiber2 = new Fiber(function (): void {
    \Fiber::suspend(2);
    trigger_error("Fatal error in fiber", E_USER_ERROR);
});

var_dump($fiber1->start());
var_dump($fiber2->start());
$fiber2->resume();

--EXPECTF--
int(1)
int(2)

Fatal error: Fatal error in fiber in %s022-fatal-error-with-multiple-fibers.php on line %d
