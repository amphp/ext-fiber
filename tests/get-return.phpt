--TEST--
Test fiber return value
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(function (): string {
    $value = Fiber::suspend("x");
    return $value;
});

$value = $fiber->start();
var_dump($value);
var_dump($fiber->resume($value . "y"));
var_dump($fiber->getReturn());

?>
--EXPECT--
string(1) "x"
NULL
string(2) "xy"
