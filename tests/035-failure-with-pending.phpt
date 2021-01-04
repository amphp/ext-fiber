--TEST--
Scheduler not finished on uncaught exception
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->delay(10, fn() => print "should not be executed");

$promise = new Failure($loop, new Exception('test'));
$promise->schedule(Fiber::this());
echo Fiber::suspend($loop->getScheduler());

--EXPECTF--
Fatal error: Uncaught Exception: test in %s.php:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
