<?php

final class Fiber
{
    public const STATUS_SUSPENDED = 0;
    public const STATUS_RUNNING = 1;
    public const STATUS_FINISHED = 2;
    public const STATUS_DEAD = 3;

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
     * @return int One of the Fiber status constants.
     */
    public function getStatus(): int { }

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
     * Private constructor to prevent userland code from throwing FiberError.
     */
    private function __construct(string $message) { }
}
