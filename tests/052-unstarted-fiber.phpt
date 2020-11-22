--TEST--
Not starting a fiber does not leak
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = Fiber::create(function (Fiber $fiber) use ($loop): void {
    Fiber::suspend(fn() => $loop->delay(10, fn() => $fiber->resume()), $loop);
});

echo "done";

--EXPECT--
done
