--TEST--
Make sure exceptions are rethrown when throwing from fiber destructor
--EXTENSIONS--
fiber
--FILE--
<?php
$fiber = new Fiber(function() {
    try {
        Fiber::suspend();
    } finally {
        throw new Exception("Exception 2");
    }
});
$fiber->start();
unset($fiber);
throw new Exception("Exception 1");
?>
--EXPECTF--
Fatal error: Uncaught FiberExit: Fiber destroyed in %s:%d
Stack trace:
#0 %s(%d): Fiber::suspend()
#1 [internal function]: {closure}()
#2 {main}

Next Exception: Exception 2 in %s:%d
Stack trace:
#0 [internal function]: {closure}()
#1 {main}
  thrown in %s on line %d
