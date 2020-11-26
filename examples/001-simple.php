<?php

class Scheduler implements FiberScheduler
{
    private string $nextId = 'a';
    private array $callbacks = [];

    public function run(): void
    {
        while (!empty($this->callbacks)) {
            foreach ($this->callbacks as $id => $callback) {
                unset($this->callbacks[$id]);
                $callback();
            }
        }
    }

    public function defer(callable $callback): void
    {
        $this->callbacks[$this->nextId++] = $callback;
    }
}

$scheduler = new Scheduler;

// Suspend the main fiber, which will be resumed by the scheduler.
$value = Fiber::suspend(fn(Fiber $fiber) => $scheduler->defer(fn() => $fiber->resume("Test")), $scheduler);

echo "After resuming main fiber: ", $value, "\n";

// Suspend the main fiber again, but this time an exception will be thrown.
Fiber::suspend(fn(Fiber $fiber) => $scheduler->defer(fn() => $fiber->throw(new Exception("Test"))), $scheduler);
