<?php

$f = new Fiber(function (int $a): int {
    $b = $a + Fiber::suspend();
    return $b * Fiber::suspend($a);
});

var_dump($f->status(), $f->start(1), $f->status(), $f->resume(2), $f->status(), $f->resume(3), $f->status());
