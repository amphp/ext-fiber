<?php

final class SchedulerFiber
{
    public function __construct(callable $callback) { }

    public function isTerminated(): bool { }
}
