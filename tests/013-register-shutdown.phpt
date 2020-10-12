--TEST--
Test await in function registered with register_shutdown_function
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

register_shutdown_function(fn() => print Fiber::await(new Success($loop, 2), $loop));

echo Fiber::await(new Success($loop, 1), $loop);

--EXPECT--
12
