<?php

function async(Loop $loop, callable $callback): Future
{
    $promise = new Promise($loop);

    $fiber = new Fiber(function () use ($promise, $callback): void {
        try {
            $promise->resolve($callback());
        } catch (\Throwable $exception) {
            $promise->fail($exception);
        }
    });

    $loop->defer(fn() => $fiber->start());

    return $promise;
}

function delay(Loop $loop, int $timeout): void
{
    $fiber = Fiber::this();
    $loop->delay($timeout, fn() => $fiber->resume());
    Fiber::suspend($loop->getScheduler());
}

function formatStacktrace(array $trace): string
{
    return \implode("\n", \array_map(static function ($e, $i) {
        $line = "#{$i} ";

        if (isset($e["file"])) {
            $line .= "{$e['file']}:{$e['line']} ";
        }

        if (isset($e["type"])) {
            $line .= $e["class"] . $e["type"];
        }

        return $line . $e["function"] . "()";
    }, $trace, \array_keys($trace)));
}
