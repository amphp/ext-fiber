<?php

final class Success implements Awaitable
{
    private Loop $loop;

    private mixed $value;

    public function __construct(Loop $loop, mixed $value = null)
    {
        if ($value instanceof Awaitable) {
            throw new \Error("Cannot use an Awaitable as success value");
        }

        $this->loop = $loop;
        $this->value = $value;
    }

    public function onResolve(callable $onResolve): void
    {
        $this->loop->defer(fn() => $onResolve(null, $this->value));
    }
}
