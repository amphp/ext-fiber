<?php

final class Failure implements Future
{
    private Loop $loop;

    private Throwable $exception;

    public function __construct(Loop $loop, Throwable $exception)
    {
        $this->loop = $loop;
        $this->exception = $exception;
    }

    public function __invoke(Fiber $fiber): void
    {
        $this->loop->defer(fn() => $fiber->throw($this->exception));
    }
}
