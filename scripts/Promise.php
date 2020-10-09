<?php

final class Promise implements Awaitable
{
    private Loop $loop;

    private array $onResolve = [];

    private Awaitable $result;

    public function __construct(Loop $loop)
    {
        $this->loop = $loop;
    }

    public function onResolve(callable $onResolve): void
    {
        if (isset($this->result)) {
            $this->result->onResolve($onResolve);
            return;
        }

        $this->onResolve[] = $onResolve;
    }

    public function resolve(mixed $value = null): void
    {
        if (isset($this->result)) {
            throw new Error("Promise already resolved");
        }

        $this->result = $value instanceof Awaitable ? $value : new Success($this->loop, $value);

        $onResolve = $this->onResolve;
        $this->onResolve = [];

        foreach ($onResolve as $callback) {
            $this->result->onResolve($callback);
        }
    }

    public function fail(Throwable $exception): void
    {
        $this->resolve(new Failure($this->loop, $exception));
    }
}
