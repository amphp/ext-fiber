--TEST--
fiber can be paused within callbacks given to internal functions.
--SKIPIF--
--FILE--
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

var_dump($f->resume(6), $f->getStatus(), $f->getReturn());


echo "Ok";
?>
--EXPECTF--
NULL
int(3)
array(3) {
  [0]=>
  int(4)
  [1]=>
  int(5)
  [2]=>
  int(6)
}
Ok