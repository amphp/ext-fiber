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

$results = [];
$results[] = $f->start(1);
$results[] = $f->resume(2);
$results[] = $f->throw($exception);
$results[] = $f->resume(3);

var_dump($results);

var_dump($f->getReturn(), $f->getStatus());
