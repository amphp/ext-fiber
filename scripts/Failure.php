<?php

final class Failure implements Awaitable
{
    private Loop $loop;

    private Throwable $exception;

    public function __construct(Loop $loop, Throwable $exception)
    {
        $this->loop = $loop;
        $this->exception = $exception;
    }

    public function onResolve(callable $onResolve): void
    {
        $this->loop->defer(fn() => $onResolve($this->exception, null));
    }
}
