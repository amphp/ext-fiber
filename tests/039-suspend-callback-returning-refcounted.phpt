--TEST--
Suspend callback returning a refcounted value does not leak
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

echo Fiber::suspend(function (Fiber $fiber) use ($loop): object {
    $loop->defer(fn() => $fiber->resume("done"));
    return new class() {};
}, $loop);

--EXPECT--
done
