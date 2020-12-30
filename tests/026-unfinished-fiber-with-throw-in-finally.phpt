--TEST--
Fiber that is never resumed with throw in finally block
--SKIPIF--
<?php
include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function () use ($loop): void {
    $fiber = new Fiber(function () use ($loop): void {
        try {
            try {
                try {
                    echo "fiber\n";
                    echo Fiber::suspend($loop->getSchedulerFiber());
                    echo "after await\n";
                } catch (Throwable $exception) {
                     echo "inner exit exception caught!\n";
                }
            } catch (Throwable $exception) {
                echo "exit exception caught!\n";
            } finally {
                echo "inner finally\n";
                throw new \Exception("finally exception");
            }
        } catch (Exception $exception) {
            echo $exception->getMessage(), "\n";
            echo \get_class($exception->getPrevious()), "\n";
        } finally {
            echo "outer finally\n";
        }

        try {
            echo Fiber::suspend($loop->getSchedulerFiber());
        } catch (FiberError $exception) {
            echo $exception->getMessage(), "\n";
        }
    });

    $fiber->start();
});

$promise = new Success($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop->getSchedulerFiber());

echo "done\n";

--EXPECT--
fiber
inner finally
finally exception
FiberExit
outer finally
Cannot suspend in a force closed fiber
done
