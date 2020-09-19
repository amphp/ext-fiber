--TEST--
fiber suspend
--SKIPIF--
--FILE--
<?php

require_once(dirname(__FILE__) . '/functions.php');

$f = new Fiber(function (int $a): int {
    return $a + Fiber::suspend();
});

//var_dump($f->getStatus(), $f->start(1), $f->getStatus(), $f->resume(2), $f->getStatus(), f->getReturn());

assert_fiber_status(Fiber::STATUS_INIT, $f);

$f->start(1);
assert_fiber_status(Fiber::STATUS_SUSPENDED, $f);

$f->resume(2);
assert_fiber_status(Fiber::STATUS_FINISHED, $f);

$result = $f->getReturn();

if ($result === 3) {
    echo "Result is correct\n";
}
else {
    echo "Result is incorrect, expected 3 but got " . var_dump($result);
}

echo "Ok";
?>
--EXPECTF--
Status is correctly 0 [STATUS_INIT]
Status is correctly 1 [STATUS_SUSPENDED]
Status is correctly 3 [STATUS_FINISHED]
Result is correct
Ok
