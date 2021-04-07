--TEST--
Suspend in force closed fiber after shutdown
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(function (): void {
    try {
        Fiber::suspend();
    } finally {
        Fiber::suspend();
    }
});

$fiber->start();

echo "done\n";

--EXPECT--
done
