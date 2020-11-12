--TEST--
Fiber without call to suspend does not leak
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function (): void {
    Fiber::run(function (): void {
        echo "no suspend\n";
    });
});

Fiber::suspend(new Success($loop), $loop);

echo "done\n";

--EXPECT--
no suspend
done
