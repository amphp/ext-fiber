--TEST--
Test throw
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;
$fiber = Fiber::this();
$loop->defer(fn() => $fiber->throw(new Exception('test')));
echo Fiber::suspend($loop->getScheduler());

--EXPECTF--
Fatal error: Uncaught Exception: test in %s.php:%d
Stack trace:
#0 %s(%d): {closure}()
#1 %s(%d): Loop->tick()
#2 %s(%d): Loop->run()
#3 [fiber function](0): Loop->{closure}()
#4 {main}
  thrown in %s on line %d
