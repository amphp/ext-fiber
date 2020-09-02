<?php

$f = new Fiber(function (int $a): int {
    $b = $a + Fiber::suspend();

    try {
        $b += Fiber::suspend($b);
    } catch (Exception $e) {
        echo "Exception caught in fiber: ", $e->getMessage(), PHP_EOL;
    }

    return $b + Fiber::suspend($b);
});

$exception = new Exception('Thrown into fiber');

$f->start(1);
$f->resume(2);
$f->throw($exception);

var_dump($f->resume(3), $f->status());
