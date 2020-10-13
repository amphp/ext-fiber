--TEST--
Finally block in a fiber that is never resumed
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
            echo Fiber::await(new Promise($loop), $loop);
        } catch (\Throwable $exception) {
            echo "exit exception caught!\n";
        } finally {
            echo "finally not reached\n";
        }

        echo "end of fiber not reached\n";
    });
});

Fiber::await(new Success($loop), $loop);

echo "done\n";

--EXPECTF--
fiber
done
