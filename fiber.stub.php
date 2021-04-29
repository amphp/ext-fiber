<?php

/** @generate-class-entries */

/** @strict-properties */
final class Fiber
{
    public function __construct(callable $callback) {}

    public function start(mixed ...$args): mixed {}

    public function resume(mixed $value = null): mixed {}

    public function throw(Throwable $exception): mixed {}

    public function isStarted(): bool {}

    public function isSuspended(): bool {}

    public function isRunning(): bool {}

    public function isTerminated(): bool {}

    public function getReturn(): mixed {}

    public static function this(): ?Fiber {}

    public static function suspend(mixed $value = null): mixed { }
}

final class ReflectionFiber
{
    public function __construct(Fiber $fiber) {}

    public function getFiber(): Fiber {}

    public function getExecutingFile(): string {}

    public function getExecutingLine(): int {}

    public function getTrace(int $options = DEBUG_BACKTRACE_PROVIDE_OBJECT): array {}

    public function getCallable(): callable;
}

final class FiberError extends Error
{
    public function __construct()
    {
        throw new \Error('The "FiberError" class is reserved for internal use and cannot be manually instantiated');
    }
}

final class FiberExit extends Exception
{
    public function __construct()
    {
        throw new \Error('The "FiberExit" class is reserved for internal use and cannot be manually instantiated');
    }
}
