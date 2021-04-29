--TEST--
Exit from fiber
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(function (): void {
    Fiber::suspend();
    echo "resumed\n";
    exit();
});

$fiber->start();

$fiber->resume();

echo "unreachable\n";

?>
--EXPECT--
resumed
