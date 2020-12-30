--TEST--
Resume running fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = Fiber::this();

$loop->defer(fn() => var_dump($fiber->isSuspended()));
$loop->defer(fn() => $fiber->resume());
echo Fiber::suspend($loop->getSchedulerFiber());

var_dump($fiber->isSuspended());
var_dump($fiber->isRunning());
var_dump($fiber->isTerminated());

$loop->defer(fn() => $fiber->resume());

--EXPECTF--
bool(true)
bool(false)
bool(true)
bool(false)

Fatal error: Uncaught FiberError: Cannot resume a fiber that is not suspended in %s:%d
Stack trace:
#0 %s(%d): Fiber->resume()
#1 %s(%d): {closure}()
#2 %s(%d): Loop->tick()
#3 %s(%d): Loop->run()
#4 [fiber function](0): Loop->{closure}()
#5 {main}
  thrown in %s on line %d
