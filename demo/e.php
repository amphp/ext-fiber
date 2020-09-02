<?php

$f = new Fiber(function (int $a): int {
    $b = $a + Fiber::suspend($a);

    try {
        $c = $b + Fiber::suspend($b);
    } finally {
        echo "Finally block executed when Fiber object destroyed", PHP_EOL;
    }

    echo "Never reached", PHP_EOL;

    return $c + Fiber::suspend($c);
});

$f->start(1);
$f->resume(2); // Enters try/finally block.

unset($f);

echo "After destroying Fiber", PHP_EOL;
