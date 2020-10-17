--TEST--
Cannot construct Fiber object
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

new Fiber;

--EXPECTF--
Fatal error: Uncaught Error: The "Fiber" class is reserved for internal use and cannot be manually instantiated in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
