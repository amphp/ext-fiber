<?php

final class ReflectionFiber
{
    public static function fromContinuation(Continuation $continuation): self { }

    public static function fromFiberScheduler(FiberScheduler $scheduler): ?self { }

    public function getExecutingFile(): string { }

    public function getExecutingLine(): int { }

    public function getThis(): ?object { }

    public function getTrace(int $options = DEBUG_BACKTRACE_PROVIDE_OBJECT): array { }

    public function isSuspended(): bool { }

    public function isRunning(): bool { }

    public function isTerminated(): bool { }

    public function isFiberScheduler(): bool { }
}
