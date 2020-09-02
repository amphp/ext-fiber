<?php

$f = new Fiber(function (int $a): int {
    $b = $a + Fiber::suspend();
    throw new Exception('Thrown from fiber');
});

$f->start(1);

try {
    $f->resume(2);; // If Fiber throws an exception, it is thrown from the resume() or throw() call.
} catch (Exception $exception) {
    echo $exception->getMessage(), PHP_EOL;
}

var_dump($f->status());
