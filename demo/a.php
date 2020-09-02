<?php

$f = new Fiber(function (int $a): int {
    return $a + Fiber::suspend();
});

var_dump($f->getStatus(), $f->start(1), $f->getStatus(), $f->resume(2), $f->getStatus(), $f->getReturn());
