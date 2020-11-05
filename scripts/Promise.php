<?php

final class Promise implements Future
{
    private Loop $loop;

    /** @var Continuation[] */
    private array $continuations = [];

    private Future $result;

    public function __construct(Loop $loop)
    {
        $this->loop = $loop;
    }

    public function __invoke(Continuation $continuation): void
    {
        if (isset($this->result)) {
            ($this->result)($continuation);
            return;
        }

        $this->continuations[] = $continuation;
    }

    public function resolve(mixed $value = null): void
    {
        if (isset($this->result)) {
            throw new Error("Promise already resolved");
        }

        $this->result = $value instanceof Future ? $value : new Success($this->loop, $value);

        $continuations = $this->continuations;
        $this->continuations = [];

        foreach ($continuations as $continuation) {
            ($this->result)($continuation);
        }
    }

    public function fail(Throwable $exception): void
    {
        $this->resolve(new Failure($this->loop, $exception));
    }
}
