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
    public function getStatus(): int { }

    /**
     * Start the Fiber by invoking the callback given to the constructor with the given arguments.
     *
     * @param mixed ...$args
     *
     * @return mixed Value given to next {@see Fiber::suspend()} call or NULL (finishes).
     *
     * @throws Throwable If the fiber throws, the exception will be thrown from this call.
     */
    public function start(mixed ...$args): mixed { }

    /**
     * @param mixed $value Value to return from {@see Fiber::suspend()}.
     *
     * @return mixed Value given to next {@see Fiber::suspend()} call or NULL if the fiber returns (finishes).
     *
     * @throws Throwable If the fiber throws, the exception will be thrown from this call.
     */
    public function resume(mixed $value = null): mixed { }

    /**
     * @param Throwable $exception Exception to throw from {@see Fiber::suspend()}.
     *
     * @return mixed Value given to next {@see Fiber::suspend()} call or NULL if the fiber returns (finishes).
     *
     * @throws Throwable If the fiber throws, the exception will be thrown from this call.
     */
    public function throw(\Throwable $exception): mixed { }

    /**
     * @return mixed Fiber return value.
     *
     * @throws Error If the fiber has not finished.
     */
    public function getReturn(): mixed { }

    /**
     * @param mixed $value Suspension value, which is then returned from {@see Fiber::resume()} or
     *                     {@see Fiber::throw()}.
     *
     * @return mixed Value given to {@see Fiber::resume()} when resuming the fiber.
     *
     * @throws Throwable Exception given to {@see Fiber::throw()}.
     * @throws Error Thrown if not within a Fiber context.
     */
    public static function suspend(mixed $value = null): mixed { }
	
	/**
	 * Returns the current Fiber context or null if not within a fiber.
	 *
	 * @return Fiber|null
	 */
	public static function getCurrent(): ?Fiber { }
}

/**
 * Exception thrown due to invalid fiber actions, such as suspending from outside a fiber.
 */
final class FiberError extends Error { }
