--TEST--
Cannot construct Continuation object
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

new Continuation;

--EXPECTF--
Fatal error: Uncaught Error: The "Continuation" class is reserved for internal use and cannot be manually instantiated in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
