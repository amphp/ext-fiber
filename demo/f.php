<?php

$f = new Fiber(function (int $a): int {
    $b = $a + Fiber::suspend();
    return $b + Fiber::suspend();
});

$exception = new Exception('Thrown into fiber');

$f->start(1);
$f->resume(2);

try {
    $f->throw($exception); // If exception is uncaught in Fiber, it is thrown from Fiber::throw()
} catch (Exception $caught) {
    var_dump($exception === $caught);
}

var_dump($f->status());
