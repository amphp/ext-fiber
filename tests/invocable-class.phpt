--TEST--
Reference to invocable class retained while running
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

class Test {
    public function __invoke() {
        $GLOBALS['test'] = null;
        var_dump($this);
        try {
            Fiber::suspend();
        } finally {
            var_dump($this);
        }
    }
}

$test = new Test;
$fiber = new Fiber($test);
$fiber->start();

?>
--EXPECT--
object(Test)#1 (0) {
}
object(Test)#1 (0) {
}
