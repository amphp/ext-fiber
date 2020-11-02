--TEST--
Fiber without call to await does not leak
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function (): void {
    Fiber::run(function (): void {
        echo "no await\n";
    });
});

Fiber::suspend(new Success($loop), $loop);

echo "done\n";

--EXPECT--
no await
done
