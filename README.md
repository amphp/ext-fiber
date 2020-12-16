# Fiber Extension

Fiber implementation for PHP using native C fibers.

## Installation

Installation of the extension is similar to other PHP extensions.

``` bash
phpize
./configure
make
make test
make install
```

## API

``` php
final class Fiber
{
    /**
     * @param callable $callback Function to invoke when starting the fiber.
     */
    public function __construct(callable $callback) { }

    /**
     * Starts execution of the fiber. Returns when the fiber suspends or terminates.
     *
     * Must be called within {@see FiberScheduler::run()}.
     *
     * @param mixed ...$args Arguments passed to fiber function.
     *
     * @throw FiberError If the fiber is running or terminated.
     * @throw Throwable If the fiber callable throws an uncaught exception.
     */
    public function start(mixed ...$args): void { }

    /**
     * Resumes the fiber, returning the given value from {@see Fiber::suspend()}.
     * Returns when the fiber suspends or terminates.
     *
     * Must be called within {@see FiberScheduler::run()}.
     *
     * @param mixed $value
     *
     * @throw FiberError If the fiber is running or terminated.
     * @throw Throwable If the fiber callable throws an uncaught exception.
     */
    public function resume(mixed $value = null): void { }

    /**
     * Throws the given exception into the fiber from {@see Fiber::suspend()}.
     * Returns when the fiber suspends or terminates.
     *
     * Must be called within {@see FiberScheduler::run()}.
     *
     * @param Throwable $exception
     *
     * @throw FiberError If the fiber is running or terminated.
     * @throw Throwable If the fiber callable throws an uncaught exception.
     */
    public function throw(Throwable $exception): void { }

    /**
     * @return bool True if the fiber has been started.
     */
    public function isStarted(): bool { }

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
     * Returns the currently executing Fiber instance.
     *
     * Cannot be called within {@see FiberScheduler::run()}.
     *
     * @return Fiber The currently executing fiber.
     *
     * @throws FiberError Thrown if within {@see FiberScheduler::run()}.
     */
    public static function this(): Fiber { }

    /**
     * Suspend execution of the fiber, switching execution to the scheduler.
     *
     * The fiber may be resumed with {@see Fiber::resume()} or {@see Fiber::throw()}
     * within the run() method of the instance of {@see FiberScheduler} given.
     *
     * Cannot be called within {@see FiberScheduler::run()}.
     *
     * @param FiberScheduler $scheduler
     *
     * @return mixed Value provided to {@see Fiber::resume()}.
     *
     * @throws FiberError Thrown if within {@see FiberScheduler::run()}.
     * @throws Throwable Exception provided to {@see Fiber::throw()}.
     */
    public static function suspend(FiberScheduler $scheduler): mixed { }
}
```

A `Fiber` object is created using `new Fiber(callable $callback)` with any callable. The callable need not call `Fiber::suspend()` directly, it may be in a deeply nested call, far down the call stack (or perhaps never call `Fiber::suspend()` at all). The returned `Fiber` may be started within a `FiberScheduler` (discussed below) using `Fiber->start(mixed ...$args)` with a variadic argument list that is provided as arguments to the callable used when creating the `Fiber`.

`Fiber::suspend()` suspends execution of the current fiber and switches to a scheduler fiber created from the instance of `FiberScheduler` given. This will be discussed in further detail in the next section describing `FiberScheduler`.

`Fiber::this()` returns the currently executing `Fiber` instance. This object may be stored to be used at a later time to resume the fiber with any value or throw an exception into the fiber. The `Fiber` object should be attached to event watchers in the `FiberScheduler` instance (event loop), added to a list of pending fibers, or otherwise used to set up logic that will resume the fiber at a later time from the instance of `FiberScheduler` provided to `Fiber::suspend()`.

A suspended fiber may be resumed in one of two ways:

* returning a value from `Fiber::suspend()` using `Fiber->resume()`
* throwing an exception from `Fiber::suspend()` using `Fiber->throw()`

### FiberScheduler

``` php
interface FiberScheduler
{
    /**
     * Run the scheduler, scheduling and responding to events.
     * This method should not return until no futher pending events remain in the fiber scheduler.
     */
    public function run(): void;
}
```

A `FiberScheduler` defines a class which is able to start new fibers using `Fiber->start()` and resume fibers using `Fiber->resume()` and `Fiber->throw()`. In general, a fiber scheduler would be an event loop that responds to events on sockets, timers, and deferred functions.

When an instance of `FiberScheduler` is provided to `Fiber::suspend()` for the first time, internally a new fiber (a scheduler fiber) is created for that instance and invokes `FiberScheduler->run()`. The scheduler fiber created is suspended when resuming or starting another fiber (that is, when calling `Fiber->start()`, `Fiber->resume()`, or `Fiber->throw()`) and again resumed when the same instance of `FiberScheduler` is provided to another call to `Fiber::suspend()`. It is expected that `FiberScheduler->run()` will not return until all pending events have been processed and any suspended fibers have been resumed. In practice this is not difficult, as the scheduler fiber is suspended when resuming a fiber and only re-entered upon a fiber suspending which will create more events in the scheduler.

If a scheduler completes (that is, returns from `FiberScheduler->run()`) without resuming the suspended fiber, an instance of `FiberError` is thrown from the call to `Fiber::suspend()`.

If a `FiberScheduler` instance whose associated fiber has completed is later reused in a call to `Fiber::suspend()`, `FiberScheduler->run()` will be invoked again to create a new fiber associated with that `FiberScheduler` instance.

`FiberScheduler->run()` throwing an exception results in an uncaught `FiberExit` exception and exits the script.

A fiber *must* be resumed from within the instance of `FiberScheduler` provided to `Fiber::suspend()`. Doing otherwise results in a fatal error. In practice this means that calling `Fiber->resume()` or `Fiber->throw()` must be within a callback registered to an event handled within a `FiberScheduler` instance. Often it is desirable to ensure resumption of a fiber is asynchronous, making it easier to reason about program state before and after an event would resume a fiber. New fibers also must be started within a `FiberScheduler` (though the started fiber may use any scheduler within that fiber).

Fibers must be started and resumed within a fiber scheduler in order to maintain ordering of the internal fiber stack. Internally, a fiber may only switch to a new fiber, a suspended fiber, or suspend to the prior fiber. In essense, a fiber may be pushed or popped from the stack, but execution cannot move to within the stack. The fiber scheduler acts as a hub which may branch into another fiber or suspend to the main fiber. If a fiber were to attempt to switch to a fiber that is already running, the program will crash. Checks prevent this from happening, throwing an exception into PHP instead of crashing the VM.

When a script ends, each scheduler fiber created from a call to `FiberScheduler->run()` is resumed in reverse order of creation to complete unfinished tasks or free resources.

### ReflectionFiber

`ReflectionFiber` is used to inspect executing fibers. A `ReflectionFiber` object can be created from any `Fiber` object, even if it has not been started or if it has been finished. This reflection class is similar to `ReflectionGenerator`.

``` php
class ReflectionFiber
{
    /**
     * @param Fiber $fiber Any Fiber object, including those that are not started or have
     *                     terminated.
     */
    public function __construct(Fiber $fiber) { }

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
     * @return bool True if the fiber has been started.
     */
    public function isStarted(): bool { }

    /**
     * @return bool True if the fiber is currently suspended.
     */
    public function isSuspended(): bool { }

    /**
     * @return bool True if the fiber is currently running.
     */
    public function isRunning(): bool { }

    /**
     * @return bool True if the fiber has completed execution (either returning or
     *              throwing an exception).
     */
    public function isTerminated(): bool { }
}
```

### ReflectionFiberScheduler

`ReflectionFiberScheduler` is used to inspect the internal fibers created from objects implementing `FiberScheduler` after being used to suspend a fiber.

``` php
class ReflectionFiberScheduler extends ReflectionFiber
{
    /**
     * @param FiberScheduler $scheduler
     */
    public function __construct(FiberScheduler $scheduler) { }

    /**
     * @return FiberScheduler The instance used to create the fiber.
     */
    public function getScheduler(): FiberScheduler { }
}
```

#### Unfinished Fibers

Fibers that are not finished (do not complete execution) are destroyed similarly to unfinished generators, executing any pending `finally` blocks. `Fiber::suspend()` may not be invoked in a force-closed fiber, just as `yield` cannot be used in a force-closed generator. Fibers are destroyed when there are no references to the `Fiber` object. An exception to this is the `{main}` fiber, where removing all references to the `Fiber` object that resumes the main fiber will result in a `FiberExit` exception to be thrown from the call to `Fiber::suspend()`, resulting in a fatal error.

#### Fiber Stacks

Each fiber is allocated a separate C stack and VM stack on the heap. The C stack is allocated using `mmap` if available, meaning physical memory is used only on demand (if it needs to be allocated to a stack value) on most platforms. Each fiber stack is allocated 1M maximum of memory by default, settable with an ini setting `fiber.stack_size`. Note that this memory is used for the C stack and is not related to the memory available to PHP code. VM stacks for each fiber are allocated in a similar way to generators and use a similar amount of memory and CPU. VM stacks are able to grow dynamically, so only a single VM page (4K) is initially allocated.

## RFC

This extension is currently being proposed for inclusion in PHP core in the [Fiber RFC](https://wiki.php.net/rfc/fibers).
