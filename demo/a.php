<?php

$f = new Fiber(function ($a) {
    return $a + Fiber::yield();
});

var_dump($f->status(), $f->start(1), $f->status(), $f->resume(2), $f->status());
