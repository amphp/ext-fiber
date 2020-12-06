--TEST--
Fiber that is never resumed with finally block
--SKIPIF--
<?php
include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function () use ($loop): void {
    Fiber::create(function () use ($loop): void {
        try {
            echo "fiber\n";
            echo $temp = Fiber::suspend($loop);
            echo "after await\n";
        } catch (Throwable $exception) {
            echo "exit exception caught!\n";
        } finally {
            echo "finally\n";
        }

        echo "end of fiber should not be reached\n";
    })->start();
});

$promise = new Success($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop);

echo "done\n";

--EXPECT--
fiber
finally
done
