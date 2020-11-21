<?php

final class ReflectionFiber
{
    /**
     * @param Fiber $fiber Any Fiber object, including those are not started or have
     *                     terminated.
     *
     * @return ReflectionFiber
     */
    public static function fromFiber(Fiber $fiber): self { }

    /**
     * @param FiberScheduler $scheduler
     *
     * @return ReflectionFiber|null Returns null if the {@see FiberScheduler} has not been
     *                              used to suspend a fiber.
     */
    public static function fromFiberScheduler(FiberScheduler $scheduler): ?self { }

    /**
     * @return string Current file of fiber execution.
     */
    public function getExecutingFile(): string { }

    /**
     * @return int Current line of fiber execution.
     */
    public function getExecutingLine(): int { }

    /**
     * @param int $options Same flags as {@see debug_backtrace()}.
     *
     * @return array Fiber backtrace, similar to {@see debug_backtrace()}
     *               and {@see ReflectionGenerator::getTrace()}.
     */
    public function getTrace(int $options = DEBUG_BACKTRACE_PROVIDE_OBJECT): array { }

    /**
     * @return bool True if the fiber is currently suspended, false otherwise.
     */
    public function isSuspended(): bool { }

    /**
     * @return bool True if the fiber is currently running, false otherwise.
     */
    public function isRunning(): bool { }

    /**
     * @return bool True if the fiber has completed execution (either returning or
     *              throwing an exception), false otherwise.
     */
    public function isTerminated(): bool { }

    /**
     * @return bool True if the fiber was created from an instance of {@see FiberScheduler}.
     */
    public function isFiberScheduler(): bool { }
}
