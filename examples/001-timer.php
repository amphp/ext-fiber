<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

var_dump(Fiber::suspend(function (Fiber $fiber) use ($loop): void {
    $loop->delay(1000, fn() => $fiber->resume(42));
}, $loop));
