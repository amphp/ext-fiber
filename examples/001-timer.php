<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

var_dump(Fiber::suspend(function (Continuation $continuation) use ($loop): void {
    $loop->delay(1000, fn() => $continuation->resume(42));
}, $loop));
