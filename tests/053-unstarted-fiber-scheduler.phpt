--TEST--
Not starting a fiber scheduler does not leak
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new FiberScheduler(fn() => null);

echo "done";

--EXPECT--
done
