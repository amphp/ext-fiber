<?php

final class Fiber
{
    /**
     * @param callable $callback Function to invoke when starting the Fiber.
     * @param mixed ...$args Function arguments.
     */
    public static function create(callable $callback, mixed ...$args): Fiber { }

    /**
     * Private constructor to force use of {@see create()}.
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
     * Resume execution of the fiber.
     *
     * @throws FiberError Thrown if the fiber has finished or is currently running.
     */
    public function resume(): void { }

    /**
     * Suspend execution of the fiber.
     *
     * @throws FiberError Thrown if not within a fiber.
     */
    public static function suspend(): void { }

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
final class FiberError extends Error
{
    /**
     * Private constructor to prevent user code from throwing FiberError.
     */
    public function __construct(string $message)
    {
        throw new \Error("FiberError cannot be constructed manually");
    }
}
