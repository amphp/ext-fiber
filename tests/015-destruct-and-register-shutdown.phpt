--TEST--
Test suspend in both object destructor and register shutdown function
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

register_shutdown_function(function () use ($loop): void {
    $promise = new Success($loop, 1);
    $promise->schedule(Fiber::this());
    echo Fiber::suspend($loop->getScheduler());
});

$object = new class($loop) {
    private Loop $loop;

    public function __construct(Loop $loop)
    {
        $this->loop = $loop;
    }

    public function __destruct()
    {
        $promise = new Promise($this->loop);
        $promise->schedule(Fiber::this());
        $this->loop->delay(10, fn() => $promise->resolve(2));
        echo Fiber::suspend($this->loop->getScheduler());
    }
};

--EXPECT--
12
