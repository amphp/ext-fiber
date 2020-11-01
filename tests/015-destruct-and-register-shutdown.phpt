--TEST--
Test await in both object destructor and register shutdown function
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

register_shutdown_function(fn() => print Fiber::await(new Success($loop, 1), $loop));


$object = new class($loop) {
    private Loop $loop;

    public function __construct(Loop $loop)
    {
        $this->loop = $loop;
    }

    public function __destruct()
    {
        $promise = new Promise($this->loop);
        $this->loop->delay(10, fn() => $promise->resolve(1));
        Fiber::await($promise, $this->loop);
        echo "2";
    }
};

--EXPECTF--
Fatal error: Uncaught FiberError: Cannot await during shutdown in %s:%d
Stack trace:
#0 %s(%d): Fiber::await(Object(Success), Object(Loop))
#1 [internal function]: {closure}()
#2 {main}
  thrown in %s on line %d

Fatal error: Uncaught FiberError: Cannot await during shutdown in %s:%d
Stack trace:
#0 %s(%d): Fiber::await(Object(Promise), Object(Loop))
#1 [internal function]: class@anonymous->__destruct()
#2 {main}
  thrown in %s on line %d
