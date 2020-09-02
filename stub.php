<?php

final class Fiber
{
    public const STATUS_INIT = 0;
    public const STATUS_SUSPENDED = 1;
    public const STATUS_RUNNING = 2;
    public const STATUS_FINISHED = 3;
    public const STATUS_DEAD = 4;

    /**
     * @param callable $callback Function to invoke when starting the Fiber.
     */
    public function __construct(callable $callback) { }

    /**
     * @return int One of the Fiber status constants.
     */
    public function status(): int { }

    /**
     * Start the Fiber by invoking the callback given to the constructor with the given arguments.
     *
     * @param mixed ...$args
     *
     * @return mixed Value given to next {@see Fiber::suspend()} call or function return value if the fiber completes
     *               execution.
     *
     * @throws Throwable If the fiber throws, the exception will be thrown from this call.
     */
    public function start(...$args) { }

    /**
     * @param mixed $value Value to return from {@see Fiber::suspend()}.
     *
     * @return mixed Value given to next {@see Fiber::suspend()} call or function return value if the fiber completes
     *               execution.
     *
     * @throws Throwable If the fiber throws, the exception will be thrown from this call.
     */
    public function resume($value = null) { }

    /**
     * @param Throwable $exception Exception to throw from {@see Fiber::suspend()}.
     *
     * @return mixed Value given to next {@see Fiber::suspend()} call or function return value if the fiber completes
     *               execution.
     *
     * @throws Throwable If the fiber throws, the exception will be thrown from this call.
     */
    public function throw(\Throwable $exception) { }

    /**
     * @param mixed $value
     *
     * @return mixed Value given to {@see Fiber::resume()} when resuming the fiber.
     *
     * @throws Error Thrown if not within a Fiber context.
     */
    public static function suspend($value = null) { }
}
