--TEST--
Fiber that is never resumed with finally block
--SKIPIF--
<?php
if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
// Throwing UnwindExit, finally blocks not executed.
echo "SKIP Throwing UnwindExit";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function () use ($loop): void {
    Fiber::run(function () use ($loop): void {
        try {
            try {
                try {
                    echo "fiber\n";
                    echo Fiber::await(new Promise($loop), $loop);
                    echo "after await\n";
                } catch (Throwable $exception) {
                     echo "inner exit exception caught!\n";
                }
            } catch (Throwable $exception) {
                echo "exit exception caught!\n";
            } finally {
                echo "inner finally\n";
            }
        } finally {
            echo "outer finally\n";
        }

        try {
            echo Fiber::await(new Promise($loop), $loop);
        } finally {
            echo "unreached\n";
        }

        echo "end of fiber should not be reached\n";
    });
});

Fiber::await(new Success($loop), $loop);

echo "done\n";

--EXPECTF--
fiber
done
inner finally
outer finally
