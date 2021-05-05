--TEST--
Exit from fiber
--EXTENSIONS--
fiber
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
