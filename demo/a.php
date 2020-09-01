<?php

$f = new Fiber(function (int $a): int {
    return $a + Fiber::suspend();
});

var_dump($f->status(), $f->start(1), $f->status(), $f->resume(2), $f->status());
