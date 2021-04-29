--TEST--
Fiber created in destructor
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

$object = new class() {
    public function __destruct()
    {
        $fiber = new Fiber(static function (): int {
            Fiber::suspend(1);
            return 2;
        });

        var_dump($fiber->start());
        var_dump($fiber->resume());
        var_dump($fiber->getReturn());
    }
};

?>
--EXPECT--
int(1)
NULL
int(2)
