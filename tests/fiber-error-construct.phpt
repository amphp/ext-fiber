--TEST--
FiberError cannot be constructed in user code
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

try {
    new FiberError;
} catch (Error $exception) {
    echo $exception->getMessage(), "\n";
}

?>
--EXPECT--
The "FiberError" class is reserved for internal use and cannot be manually instantiated
