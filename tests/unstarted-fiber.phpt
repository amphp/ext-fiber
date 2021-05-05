--TEST--
Not starting a fiber does not leak
--EXTENSIONS--
fiber
--FILE--
<?php

$fiber = new Fiber(fn() => null);

echo "done";

?>
--EXPECT--
done
