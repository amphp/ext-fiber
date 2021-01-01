--TEST--
Test FiberScheduler instance is run to completion on shutdown
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$promise = new Success($loop);
$promise->schedule(Fiber::this());

Fiber::suspend($loop->getScheduler());

$loop->defer(fn() => print 'test');

--EXPECT--
test
