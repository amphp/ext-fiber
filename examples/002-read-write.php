<?php

class Scheduler implements FiberScheduler
{
    private string $nextId = 'a';
    private array $deferCallbacks = [];
    private array $read = [];
    private array $streamCallbacks = [];

    public function run(): void
    {
        while (!empty($this->deferCallbacks) || !empty($this->read)) {
            foreach ($this->deferCallbacks as $id => $defer) {
                unset($this->deferCallbacks[$id]);
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

[$read, $write] = \stream_socket_pair(
    \stripos(PHP_OS, 'win') === 0 ? STREAM_PF_INET : STREAM_PF_UNIX,
    STREAM_SOCK_STREAM,
    STREAM_IPPROTO_IP
);

$scheduler = new Scheduler;

// Read data in a separate fiber after checking if the stream is readable.
$fiber = Fiber::create(function () use ($scheduler, $read): void {
    echo "Waiting for data...\n";

    \Fiber::suspend(
        fn(Fiber $fiber) => $scheduler->read($read, fn() => $fiber->resume()),
        $scheduler
    );

    $data = \fread($read, 8192);

    echo "Received data: ", $data, "\n";
});

$scheduler->defer(fn() => $fiber->start());

// Suspend main fiber to enter the scheduler.
echo Fiber::suspend(
    fn(Fiber $fiber) => $scheduler->defer(fn() => $fiber->resume("Writing data...\n")),
    $scheduler
);

// Write data in main thread once it is resumed.
\fwrite($write, "Hello, world!");
