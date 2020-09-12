<?php

final class Fiber
{
    /**
     * @param callable $callback Function to invoke when starting the Fiber.
     * @param mixed ...$args Function arguments.
     */
    public static function run(callable $callback, mixed ...$args): void { }

    /**
     * Private constructor to force use of {@see run()}.
     */
    private function __construct() { }

    /**
     * @return bool True if the fiber is suspended.
     */
    public function isSuspended(): bool { }

    /**
     * @return bool True if the fiber is currently running.
     */
    public function isRunning(): bool { }

    /**
     * @return bool True if the fiber has completed execution.
     */
    public function isTerminated(): bool { }

    /**
     * Resume execution of the fiber, returning the given value from {@see Fiber::suspend()}.
     *
     * @param mixed $value
     *
     * @throws FiberError Thrown if the fiber has finished or is currently running.
     */
    public function resume(mixed $value): void { }

    /**
     * Throw an exception from {@see Fiber::suspend()}.
     *
     * @param Throwable $exception
     *
     * @throws FiberError Thrown if the fiber has finished or is currently running.
     */
    public function throw(\Throwable $exception): void { }

    /**
     * Suspend execution of the fiber.
     *
     * @param Future $future
     *
     * @return mixed Value given to {@see Fiber::resume()}.
     *
     * @throws FiberError Thrown if not within a fiber context.
     */
    public static function suspend(Future $future): mixed { }

    /**
     * @return bool True if currently executing within a fiber context, false if in root context.
     */
    public static function inFiber(): bool { }
}
