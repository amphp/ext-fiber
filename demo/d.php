<?php

// Fibers can be paused within callbacks given to internal functions.

$f = new Fiber(function (array $values): array {
    return array_map(function (int $value): int {
        return Fiber::suspend($value);
    }, $values);
});

$f->start([1, 2, 3]);
$f->resume(4);
$f->resume(5);

var_dump($f->resume(6), $f->status(), $f->getReturn());
