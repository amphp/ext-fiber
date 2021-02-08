--TEST--
Arguments to fiber callback
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(function (int $x): int {
    return $x + Fiber::suspend($x);
});

$x = $fiber->start(1);
$fiber->resume(0);
var_dump($fiber->getReturn());

$fiber = new Fiber(function (int $x): int {
    return $x + Fiber::suspend($x);
});

$fiber->start('test');

--EXPECTF--
int(1)

Fatal error: Uncaught TypeError: {closure}(): Argument #1 ($x) must be of type int, string given in %s014-start-arguments.php:%d
Stack trace:
#0 [fiber function](0): {closure}('test')
#1 {main}
  thrown in %s014-start-arguments.php on line %d
