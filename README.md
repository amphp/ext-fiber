# Fiber Extension

Fiber implementation for PHP using native C fibers.

### Requirements

- PHP 8+

## Installation

Installation of the extension is similar to other PHP extensions.

``` bash
git clone https://github.com/amphp/ext-fiber
cd ext-fiber
phpize
./configure
make
make test
make install
```

## API

Fibers are made by creating an instance of the `Fiber` class.

``` php
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
    public static function this(): ?self {}

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
```

A `Fiber` object is created using `new Fiber(callable $callback)` with any callable. The callable need not call `Fiber::suspend()` directly, it may be in a deeply nested call, far down the call stack (or perhaps never call `Fiber::suspend()` at all). The returned `Fiber` may be started using `Fiber->start(mixed ...$args)` with a variadic argument list that is provided as arguments to the callable used when creating the `Fiber`.

`Fiber::suspend()` suspends execution of the current fiber and returns execution to the call to `Fiber->start()`, `Fiber->resume()`, or `Fiber->throw()`. The value passed to `Fiber::suspend()` is used as the return value of these methods. If the fiber throws an exception, the exception is thrown from the call to these methods. `Fiber::suspend()` will throw an instance of `FiberError` if called outside a fiber (i.e., if called from `{main}`).

A suspended fiber may be resumed in one of two ways:

* returning a value from `Fiber::suspend()` using `Fiber->resume()`
* throwing an exception from `Fiber::suspend()` using `Fiber->throw()`

`Fiber->getReturn()` returns the value returned from a terminated fiber (`NULL` is returned if the fiber did not return a value). This function will throw an instance of `FiberError` if the fiber has not completed execution or threw an exception.

`Fiber::this()` returns the currently executing `Fiber` instance or `NULL` if called from `{main}`. This allows a fiber to store a reference to itself elsewhere, such as within an event loop callback or an array of awaiting fibers.

---

### ReflectionFiber

`ReflectionFiber` is used to inspect executing fibers. A `ReflectionFiber` object can be created from any `Fiber` object, even if it has not been started or if it has been finished. This reflection class is similar to `ReflectionGenerator`.

``` php
final class ReflectionFiber
{
    /**
     * @param Fiber $fiber Any Fiber object, including those that are not started or have
     *                     terminated.
     */
    public function __construct(Fiber $fiber) {}

    /**
     * @return Fiber The reflected Fiber object.
     */
    public function getFiber(): Fiber {}

    /**
     * @return string Current file of fiber execution.
     */
    public function getExecutingFile(): string {}

    /**
     * @return int Current line of fiber execution.
     */
    public function getExecutingLine(): int {}

    /**
     * @param int $options Same flags as {@see debug_backtrace()}.
     *
     * @return array Fiber backtrace, similar to {@see debug_backtrace()}
     *               and {@see ReflectionGenerator::getTrace()}.
     */
    public function getTrace(int $options = DEBUG_BACKTRACE_PROVIDE_OBJECT): array {}

    /**
     * @return bool True if the fiber has been started.
     */
    public function isStarted(): bool {}

    /**
     * @return bool True if the fiber is currently suspended.
     */
    public function isSuspended(): bool {}

    /**
     * @return bool True if the fiber is currently running.
     */
    public function isRunning(): bool {}

    /**
     * @return bool True if the fiber has completed execution (either returning or
     *              throwing an exception), false otherwise.
     */
    public function isTerminated(): bool {}
}
```

#### Unfinished Fibers

Fibers that are not finished (do not complete execution) are destroyed similarly to unfinished generators, executing any pending `finally` blocks. `Fiber::suspend()` may not be invoked in a force-closed fiber, just as `yield` cannot be used in a force-closed generator. Fibers are destroyed when there are no references to the `Fiber` object.

#### Fiber Stacks

Each fiber is allocated a separate C stack and VM stack on the heap. The C stack is allocated using `mmap` if available, meaning physical memory is used only on demand (if it needs to be allocated to a stack value) on most platforms. Each fiber stack is allocated a maximum of 8M of memory by default, settable with an ini setting `fiber.stack_size`. Note that this memory is used for the C stack and is not related to the memory available to PHP code. VM stacks for each fiber are allocated in a similar way to generators and use a similar amount of memory and CPU. VM stacks are able to grow dynamically, so only a single VM page (4K) is initially allocated.

## RFC

This extension is currently being proposed for inclusion in PHP core in the [Fiber RFC](https://wiki.php.net/rfc/fibers).
