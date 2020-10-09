<?php

final class TimerQueueEntry
{
    public string $id;

    public int $expiration;

    public $callback;

    public function __construct(string $id, callable $callback, int $expiration)
    {
        $this->id = $id;
        $this->callback = $callback;
        $this->expiration = $expiration;
    }
}
