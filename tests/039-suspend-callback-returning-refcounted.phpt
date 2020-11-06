--TEST--
Suspend callback returning a refcounted value does not leak
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

echo Fiber::suspend(function (Continuation $continuation) use ($loop): object {
    $loop->defer(fn() => $continuation->resume("done"));
    return new class() {};
}, $loop);

--EXPECT--
done
