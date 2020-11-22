--TEST--
ReflectionFiber::fromFiberScheduler()
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

var_dump(ReflectionFiber::fromFiberScheduler($loop));

Fiber::suspend(new Success($loop), $loop);

$reflection = ReflectionFiber::fromFiberScheduler($loop);

var_dump($reflection->getExecutingFile());
var_dump($reflection->getExecutingLine());

--EXPECTF--
NULL
string(%d) "%s%escripts%eSuccess.php"
int(21)
