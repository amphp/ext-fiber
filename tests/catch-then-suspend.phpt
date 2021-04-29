--TEST--
Catch exception thrown into fiber, then suspend again
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(function () {
    try {
        Fiber::suspend('in try');
    } catch (Exception $exception) {
    }

    Fiber::suspend('after catch');
});

var_dump($fiber->start());

var_dump($fiber->throw(new Exception));

var_dump($fiber->resume());

?>
--EXPECT--
string(6) "in try"
string(11) "after catch"
NULL
