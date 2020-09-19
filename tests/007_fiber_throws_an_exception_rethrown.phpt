--TEST--
fiber If Fiber throws an exception, it is thrown from the resume() or throw()
--SKIPIF--
--FILE--
<?php

$f = new Fiber(function (int $a): int {
    $b = $a + Fiber::suspend();
    throw new Exception('Thrown from fiber');
});

$f->start(1);

try {
    $f->resume(2);; // If Fiber throws an exception, it is thrown from the resume() or throw() call.
} catch (Exception $exception) {
    echo $exception->getMessage(), PHP_EOL;
}

if ($f->getStatus() === Fiber::STATUS_DEAD) {
    echo "Fiber was correctly killed.\n";
}
else {
    printf(
        "fiber status was not %s but instead %s\n",
        Fiber::STATUS_DEAD,
        $f->getStatus()
    );
    exit(-1);
}
echo "Ok";

?>
--EXPECTF--
Thrown from fiber
Fiber was correctly killed.
Ok