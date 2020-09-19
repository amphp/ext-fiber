--TEST--
fiber Exception Thrown into fiber
--SKIPIF--
--FILE--
<?php


$f = new Fiber(function (int $a): int {
    $b = $a + Fiber::suspend();
    return $b + Fiber::suspend();
});

$exception = new Exception('Thrown into fiber');

$f->start(1);
$f->resume(2);

try {
    $f->throw($exception); // If exception is uncaught in Fiber, it is thrown from Fiber::throw()
} catch (Exception $caught) {
    var_dump($exception === $caught);
}

var_dump($f->getStatus());

echo "Ok";

?>
--EXPECTF--
bool(true)
int(4)
Ok