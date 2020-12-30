<?php

final class Loop
{
    private const CHUNK_SIZE = 8192;

    private string $nextId = 'a';

    /** @var callable[] */
    private array $deferQueue = [];

    /** @var resource[] */
    private array $read = [];

    /** @var resource[] */
    private array $write = [];

    /** @var callable[] */
    private array $streamCallbacks = [];

    /** @var string[] */
    private array $writeData = [];

    private TimerQueue $timerQueue;

    private bool $running = false;

    private SchedulerFiber $scheduler;

    public function __construct()
    {
        $this->timerQueue = new TimerQueue;
    }

    public function getSchedulerFiber(): SchedulerFiber
    {
        if (!isset($this->scheduler) || $this->scheduler->isTerminated()) {
            $this->scheduler = new SchedulerFiber(fn() => $this->run());
        }

        return $this->scheduler;
    }

    public function run(): void
    {
        $this->running = true;

        try {
            while ($this->running) {
                if ($this->isEmpty()) {
                    return;
                }

                $this->tick();
            }
        } finally {
            $this->running = false;
        }
    }

    public function isRunning(): bool
    {
        return $this->running;
    }

    private function isEmpty(): bool
    {
        return empty($this->deferQueue)
            && empty($this->read)
            && empty($this->write)
            && $this->timerQueue->peek() === null;
    }

    private function tick(): void
    {
        $deferQueue = $this->deferQueue;
        $this->deferQueue = [];

        try {
            foreach ($deferQueue as $id => $defer) {
                unset($deferQueue[$id]);
                $defer();
            }
        } finally {
            $this->deferQueue = array_merge($deferQueue, $this->deferQueue);
        }

        $timeout = empty($this->deferQueue) ? $this->getTimeout() : 0;

        $this->select($this->read, $this->write, $timeout);

        while ($timer = $this->timerQueue->extract($this->now())) {
            $timer();
        }
    }

    private function select(array $read, array $write, int $timeout): void
    {
        if (empty($read) && empty($write)) {
            if ($timeout >= 0) {
                usleep($timeout * 1000); // Milliseconds to microseconds.
            }
            return;
        }

        if ($timeout >= 0) {
            $timeout /= 1000; // Milliseconds to seconds.
            $seconds = (int) $timeout;
            $microseconds = (int) (($timeout - $seconds) * 1000);
        } else {
            $seconds = null;
            $microseconds = null;
        }

        $except = null;

        if (!stream_select($read, $write, $except, $seconds, $microseconds)) {
            return;
        }

        foreach ($read as $id => $resource) {
            $callback = $this->streamCallbacks[$id];
            unset($this->read[$id], $this->streamCallbacks[$id]);

            $data = \fread($resource, self::CHUNK_SIZE);
            if ($data === false) {
                $data = null;
            }

            $callback($data);
        }

        foreach ($write as $id => $resource) {
            $callback = $this->streamCallbacks[$id];
            $data = $this->writeData[$id];
            unset($this->write[$id], $this->writeData[$id], $this->streamCallbacks[$id]);

            $length = \fwrite($resource, $data, self::CHUNK_SIZE);

            $callback((int) $length);
        }
    }

    public function now(): int
    {
        return (int) (microtime(true) * 1000);
    }

    public function defer(callable $callback): string
    {
        $id = $this->nextId++;
        $this->deferQueue[$id] = $callback;
        return $id;
    }

    public function delay(int $timeout, callable $callback): string
    {
        $id = $this->nextId++;
        $this->timerQueue->insert($id, $callback, $this->now() + $timeout);
        return $id;
    }

    public function read($resource, callable $callback): string
    {
        $id = $this->nextId++;
        \stream_set_blocking($resource, false);
        $this->read[$id] = $resource;
        $this->streamCallbacks[$id] = $callback;
        return $id;
    }

    public function write($resource, string $data, callable $callback): string
    {
        $id = $this->nextId++;
        \stream_set_blocking($resource, false);
        $this->write[$id] = $resource;
        $this->writeData[$id] = $data;
        $this->streamCallbacks[$id] = $callback;
        return $id;
    }

    private function getTimeout(): int
    {
        $expiration = $this->timerQueue->peek();

        if ($expiration === null) {
            return -1;
        }

        $expiration -= $this->now();

        return $expiration > 0 ? $expiration : 0;
    }
}
