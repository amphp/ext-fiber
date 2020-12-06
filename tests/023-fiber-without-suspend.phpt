--TEST--
Fiber without call to suspend does not leak
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$loop->defer(function (): void {
    Fiber::create(function (): void {
        echo "no suspend\n";
    })->start();
});

$promise = new Success($loop);
$promise->schedule(Fiber::this());
Fiber::suspend($loop);

echo "done\n";

--EXPECT--
no suspend
done
