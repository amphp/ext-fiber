<?php

class EventScheduler
{
    private string $nextId = 'a';
    private array $deferCallbacks = [];
    private array $read = [];
    private array $streamCallbacks = [];

    public function run(): void
    {
        while (!empty($this->deferCallbacks) || !empty($this->read)) {
            $defers = $this->deferCallbacks;
            $this->deferCallbacks = [];
            foreach ($defers as $id => $defer) {
                $defer();
            }

            $this->select($this->read);
        }
    }

    private function select(array $read): void
    {
        $timeout = empty($this->deferCallbacks) ? null : 0;
        if (!stream_select($read, $write, $except, $timeout, $timeout)) {
            return;
        }

        foreach ($read as $id => $resource) {
            $callback = $this->streamCallbacks[$id];
            unset($this->read[$id], $this->streamCallbacks[$id]);
            $callback($resource);
        }
    }
    public function defer(callable $callback): void
    {
        $id = $this->nextId++;
        $this->deferCallbacks[$id] = $callback;
    }

    public function read($resource, callable $callback): void
    {
        $id = $this->nextId++;
        $this->read[$id] = $resource;
        $this->streamCallbacks[$id] = $callback;
    }
}

[$read, $write] = stream_socket_pair(
    stripos(PHP_OS, 'win') === 0 ? STREAM_PF_INET : STREAM_PF_UNIX,
    STREAM_SOCK_STREAM,
    STREAM_IPPROTO_IP
);

// Set streams to non-blocking mode.
stream_set_blocking($read, false);
stream_set_blocking($write, false);

$eventScheduler = new EventScheduler;
$schedulerFiber = new SchedulerFiber(fn() => $eventScheduler->run());

// Read data in a separate fiber after checking if the stream is readable.
$fiber = new Fiber(function () use ($eventScheduler, $schedulerFiber, $read): void {
    echo "Waiting for data...\n";

    $fiber = Fiber::this();
    $eventScheduler->read($read, fn() => $fiber->resume());
    Fiber::suspend($schedulerFiber);

    $data = fread($read, 8192);

    echo "Received data: ", $data, "\n";
});

// Start the new fiber within the fiber scheduler.
$eventScheduler->defer(fn() => $fiber->start());

// Suspend main fiber to enter the scheduler.
$fiber = Fiber::this();
$eventScheduler->defer(fn() => $fiber->resume("Writing data...\n"));
echo Fiber::suspend($schedulerFiber);

// Write data in main thread once it is resumed.
fwrite($write, "Hello, world!");
