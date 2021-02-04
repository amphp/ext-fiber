--TEST--
Test resume
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(function () {
    $value = Fiber::suspend('test');
    echo $value;
});

$value = $fiber->start();
$fiber->resume($value);

--EXPECT--
test
