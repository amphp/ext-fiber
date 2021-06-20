<?php

final class Fiber
{
    /**
     * @param callable $callback Function to invoke when starting the fiber.
     */
    public function __construct(callable $callback) {}

    /**
     * Starts execution of the fiber. Returns when the fiber suspends or terminates.
     *
     * @param mixed ...$args Arguments passed to fiber function.
     *
     * @return mixed Value from the first suspension point or NULL if the fiber returns.
     *
     * @throw FiberError If the fiber has already been started.
     * @throw Throwable If the fiber callable throws an uncaught exception.
     */
    public function start(mixed ...$args): mixed {}

    /**
     * Resumes the fiber, returning the given value from {@see Fiber::suspend()}.
     * Returns when the fiber suspends or terminates.
     *
     * @param mixed $value
     *
     * @return mixed Value from the next suspension point or NULL if the fiber returns.
     *
     * @throw FiberError If the fiber has not started, is running, or has terminated.
     * @throw Throwable If the fiber callable throws an uncaught exception.
     */
    public function resume(mixed $value = null): mixed {}

    /**
     * Throws the given exception into the fiber from {@see Fiber::suspend()}.
     * Returns when the fiber suspends or terminates.
     *
     * @param Throwable $exception
     *
     * @return mixed Value from the next suspension point or NULL if the fiber returns.
     *
     * @throw FiberError If the fiber has not started, is running, or has terminated.
     * @throw Throwable If the fiber callable throws an uncaught exception.
     */
    public function throw(Throwable $exception): mixed {}

    /**
     * @return bool True if the fiber has been started.
     */
    public function isStarted(): bool {}

    /**
     * @return bool True if the fiber is suspended.
     */
    public function isSuspended(): bool {}

    /**
     * @return bool True if the fiber is currently running.
     */
    public function isRunning(): bool {}

    /**
     * @return bool True if the fiber has completed execution (returned or threw).
     */
    public function isTerminated(): bool {}

    /**
     * @return mixed Return value of the fiber callback. NULL is returned if the fiber does not have a return statement.
     *
     * @throws FiberError If the fiber has not terminated or the fiber threw an exception.
     */
    public function getReturn(): mixed {}

    /**
     * @return self|null Returns the currently executing fiber instance or NULL if in {main}.
     */
    public static function getCurrent(): ?self {}

    /**
     * Suspend execution of the fiber. The fiber may be resumed with {@see Fiber::resume()} or {@see Fiber::throw()}.
     *
     * Cannot be called from {main}.
     *
     * @param mixed $value Value to return from {@see Fiber::resume()} or {@see Fiber::throw()}.
     *
     * @return mixed Value provided to {@see Fiber::resume()}.
     *
     * @throws FiberError Thrown if not within a fiber (i.e., if called from {main}).
     * @throws Throwable Exception provided to {@see Fiber::throw()}.
     */
    public static function suspend(mixed $value = null): mixed {}
}
