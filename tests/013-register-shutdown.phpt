--TEST--
Test suspend in function registered with register_shutdown_function
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

register_shutdown_function(fn() => print Fiber::suspend(new Success($loop, 'shutdown'), $loop));

Fiber::suspend(new Success($loop), $loop);

--EXPECT--
shutdown
