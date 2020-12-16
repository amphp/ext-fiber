<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

[$read, $write] = stream_socket_pair(
    stripos(PHP_OS, 'win') === 0 ? STREAM_PF_INET : STREAM_PF_UNIX,
    STREAM_SOCK_STREAM,
    STREAM_IPPROTO_IP
);

// Set streams to non-blocking mode.
stream_set_blocking($read, false);
stream_set_blocking($write, false);

$loop = new Loop;

// Write data in a separate fiber after a 1 second delay.
$fiber = new Fiber(function () use ($loop, $write): void {
    $fiber = Fiber::this();

    // Suspend fiber for 1 second.
    echo "Waiting for 1 second...\n";
    $loop->delay(1000, fn() => $fiber->resume());
    Fiber::suspend($loop);

    // Write data to the socket once it is writable.
    echo "Writing data...\n";
    $loop->write($write, 'Hello, world!', fn(int $bytes) => $fiber->resume($bytes));
    $bytes = Fiber::suspend($loop);

    echo "Wrote {$bytes} bytes.\n";
});

$loop->defer(fn() => $fiber->start());

echo "Waiting for data...\n";

// Read data in main fiber.
$fiber = Fiber::this();
$loop->read($read, fn(?string $data) => $fiber->resume($data));
$data = Fiber::suspend($loop);

echo "Received data: ", $data, "\n";
