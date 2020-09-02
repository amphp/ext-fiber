<?php

$f = new Fiber(function (int $a): int {
    $b = $a + Fiber::suspend();
    return $b * Fiber::suspend($a);
});

var_dump($f->getStatus(), $f->start(1), $f->getStatus(), $f->resume(2), $f->getStatus(), $f->resume(3), $f->getStatus());
