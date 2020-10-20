<?php

final class Loop implements FiberScheduler
{
    private string $nextId = 'a';

    /** @var callable[] */
    private array $deferQueue = [];

    private TimerQueue $timerQueue;

    private bool $running = false;

    public function __construct()
    {
        $this->timerQueue = new TimerQueue;
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
        return empty($this->deferQueue) && $this->timerQueue->peek() === null;
    }

    private function tick(): void
    {
        $now = $this->now();

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

        $timeout = $this->getTimeout();

        if (empty($this->deferQueue) && $timeout > 0) {
            usleep((int) ($timeout * 1000));
        }

        while ($timer = $this->timerQueue->extract($now)) {
            $timer();
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
