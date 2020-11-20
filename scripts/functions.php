<?php

function async(Loop $loop, callable $callback): Future
{
    $promise = new Promise($loop);

    $loop->defer(fn() => \Fiber::run(function () use ($promise, $callback): void {
        try {
            $promise->resolve($callback());
        } catch (\Throwable $exception) {
            $promise->fail($exception);
        }
    }));

    return $promise;
}

function delay(Loop $loop, int $timeout): void
{
    \Fiber::suspend(fn(Continuation $continuation) => $loop->delay($timeout, fn() => $continuation->resume()), $loop);
}

function createSocketPair(): array
{
    $sockets = \stream_socket_pair(
        \stripos(PHP_OS, 'win') === 0 ? STREAM_PF_INET : STREAM_PF_UNIX,
        STREAM_SOCK_STREAM,
        STREAM_IPPROTO_IP
    );

    if ($sockets === false) {
        throw new Exception('Failed to create socket pair');
    }

    return $sockets;
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
