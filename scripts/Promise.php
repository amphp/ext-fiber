<?php

final class Promise implements Future
{
    private Loop $loop;

    /** @var Fiber[] */
    private array $fibers = [];

    private Future $result;

    public function __construct(Loop $loop)
    {
        $this->loop = $loop;
    }

    public function schedule(Fiber $fiber): void
    {
        if (isset($this->result)) {
            $this->result->schedule($fiber);
            return;
        }

        $this->fibers[] = $fiber;
    }

    public function resolve(mixed $value = null): void
    {
        if (isset($this->result)) {
            throw new Error("Promise already resolved");
        }

        $this->result = $value instanceof Future ? $value : new Success($this->loop, $value);

        $fibers = $this->fibers;
        $this->fibers = [];

        foreach ($fibers as $fiber) {
            $this->result->schedule($fiber);
        }
    }

    public function fail(Throwable $exception): void
    {
        $this->resolve(new Failure($this->loop, $exception));
    }
}
