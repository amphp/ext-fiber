--TEST--
Fiber that is never resumed
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function () use ($loop): void {
    Fiber::run(function () use ($loop): void {
        try {
            echo "fiber\n";
            echo Fiber::suspend(new Promise($loop), $loop);
            echo "after await\n";
        } catch (Throwable $exception) {
            echo "exit exception caught!\n";
        }

        echo "end of fiber should not be reached\n";
    });
});

Fiber::suspend(new Success($loop), $loop);

echo "done\n";

--EXPECTF--
fiber
done
