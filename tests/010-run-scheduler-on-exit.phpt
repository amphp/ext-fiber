--TEST--
Test FiberScheduler instance is run to completion on shutdown
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

Fiber::await(new Success($loop), $loop);

$loop->defer(fn() => print 'test');

--EXPECT--
test
