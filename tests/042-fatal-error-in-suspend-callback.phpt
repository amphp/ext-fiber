--TEST--
Fatal error in suspend callback
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

Fiber::suspend(fn() => trigger_error("Fatal error in suspend callback", E_USER_ERROR), new Loop);

--EXPECTF--
Fatal error: Fatal error in suspend callback in %s on line %d
