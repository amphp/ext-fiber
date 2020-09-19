--TEST--
fiber suspend variant 2
--SKIPIF--
--FILE--
<?php

require_once(dirname(__FILE__) . '/functions.php');

$f = new Fiber(function (int $a): int {
    $b = $a + Fiber::suspend();
    return $b * Fiber::suspend($a);
});

//var_dump($f->getStatus(), $f->start(1), $f->getStatus(), $f->resume(2), $f->getStatus(), $f->resume(3), $f->getStatus());

assert_fiber_status(Fiber::STATUS_INIT, $f);
$f->start(1);
assert_fiber_status(Fiber::STATUS_SUSPENDED, $f);
$f->resume(2);
assert_fiber_status(Fiber::STATUS_SUSPENDED, $f);
$f->resume(3);
assert_fiber_status(Fiber::STATUS_FINISHED, $f);

$result = $f->getReturn();

$expectedResult = 9;

if ($result === $expectedResult) {
    echo "Result is correct\n";
}
else {
    echo "Result is incorrect, expected $expectedResult but got ";
    var_dump($result);
    echo "\n";
}

echo "Ok";
?>
--EXPECTF--
Status is correctly 0 [STATUS_INIT]
Status is correctly 1 [STATUS_SUSPENDED]
Status is correctly 1 [STATUS_SUSPENDED]
Status is correctly 3 [STATUS_FINISHED]
Result is correct
Ok