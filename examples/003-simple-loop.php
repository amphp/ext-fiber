<?php

class Loop
{
    /**
     * @var Fiber[]
     */
    private array $fibers;

    /**
     * Run the scheduler.
     */
    public function run(): void
    {
        while (!empty($this->fibers)) {
            $fibers = $this->fibers;
            $this->fibers = [];
            foreach ($fibers as $id => $fiber) {
                if ($fiber->isSuspended()) {
                    $fiber->resume();
                }
                else {
                    $fiber->start();
                }

                if ($fiber->isSuspended()) {
                    $this->fibers[$id] = $fiber;
                }
            }
        }
    }

    /**
     * Enqueue a fiber to executed at a later time.
     * @param callable $callback
     * @return Fiber
     */
    public function defer(callable $callback): Fiber
    {
        $fiber = new Fiber($callback);

        $this->fibers[spl_object_hash($fiber)] = $fiber;

        return $fiber;
    }
}

$loop = new Loop;

$one = $loop->defer(function () {
    echo "start 1\n";
    return 1;
});
$two = $loop->defer(function () use ($loop) {
    echo "start 2\n";
    $loop->defer(function () {
        echo "nested fiber\n";
    });
    echo "suspend 2\n";
    Fiber::suspend();
    echo "resume 2\n";
    return 2;
});
$three = $loop->defer(function () {
    echo "start 3\n";
    return 3;
});

$loop->run();

echo "return ", $one->getReturn(), "\n";
echo "return ", $two->getReturn(), "\n";
echo "return ", $three->getReturn(), "\n";
