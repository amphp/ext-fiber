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

    public function __invoke(Continuation $continuation): void
    {
        $this->loop->defer(fn() => $continuation->throw($this->exception));
    }
}
