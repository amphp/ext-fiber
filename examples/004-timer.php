<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;
$fiber = Fiber::this();
$loop->delay(1000, fn() => $fiber->resume(42));
var_dump(Fiber::suspend($loop));
