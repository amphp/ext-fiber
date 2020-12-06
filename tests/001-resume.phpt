--TEST--
Test resume
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;
$fiber = Fiber::this();
$loop->defer(fn() => $fiber->resume('test'));
echo Fiber::suspend($loop);

--EXPECT--
test
