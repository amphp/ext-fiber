--TEST--
Not starting a fiber does not leak
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$fiber = new Fiber(fn() => null);

echo "done";

?>
--EXPECT--
done
