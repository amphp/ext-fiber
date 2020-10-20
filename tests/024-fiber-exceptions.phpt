--TEST--
Fiber exception classes cannot be constructed in user code
--SKIPIF--
<?php if (!extension_loaded('fiber')) echo "ext-fiber not loaded";
--FILE--
<?php

try {
    new FiberError;
} catch (Error $exception) {
    echo $exception->getMessage(), "\n";
}

try {
    new FiberExit;
} catch (Error $exception) {
    echo $exception->getMessage(), "\n";
}

--EXPECT--
The "FiberError" class is reserved for internal use and cannot be manually instantiated
The "FiberExit" class is reserved for internal use and cannot be manually instantiated
