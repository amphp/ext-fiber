<?php

class Scheduler implements FiberScheduler
{
    private string $nextId = 'a';
    private array $callbacks = [];

    /**
     * Run the scheduler.
     */
    public function run(): void
    {
        while (!empty($this->callbacks)) {
            $callbacks = $this->callbacks;
            $this->callbacks = [];
            foreach ($callbacks as $id => $callback) {
                $callback();
            }
        }
    }

    /**
     * Enqueue a callback to executed at a later time.
     */
    public function defer(callable $callback): void
    {
        $this->callbacks[$this->nextId++] = $callback;
    }
}

$scheduler = new Scheduler;

// Get a reference to the currently executing fiber ({main} in this case).
$fiber = Fiber::this();

// Suspend the main fiber, which will be resumed by the scheduler.
$scheduler->defer(fn() => $fiber->resume("Test"));
$value = Fiber::suspend($scheduler);

echo "After resuming main fiber: ", $value, "\n";

// Suspend the main fiber again, but this time an exception will be thrown.
$scheduler->defer(fn() => $fiber->throw(new Exception("Test")));
Fiber::suspend($scheduler);
