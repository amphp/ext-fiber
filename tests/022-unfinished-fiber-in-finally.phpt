--TEST--
Fiber that is never resumed with finally block
--SKIPIF--
<?php
if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function () use ($loop): void {
    Fiber::run(function () use ($loop): object {
        try {
            try {
                echo "fiber\n";
                return new \stdClass;
            } finally {
                echo "inner finally\n";
                Fiber::await(new Promise($loop), $loop);
                echo "after await\n";
            }
        } catch (Throwable $exception) {
            echo "exit exception caught!\n";
        } finally {
            echo "outer finally\n";
        }

        echo "end of fiber should not be reached\n";
    });
});

Fiber::await(new Success($loop), $loop);

echo "done\n";

--EXPECTF--
fiber
inner finally
done
outer finally
