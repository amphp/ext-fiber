<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

[$read, $write] = createSocketPair();

$loop = new Loop;

// Write data in a separate fiber after a 1 second delay.
$fiber = Fiber::create(function () use ($loop, $write): void {
    \Fiber::suspend(function (Fiber $fiber) use ($loop): void {
        $loop->delay(1000, fn() => $fiber->resume());
    }, $loop);

    \Fiber::suspend(function (Fiber $fiber) use ($loop, $write): void {
        $loop->write($write, 'Hello, world!', fn(int $bytes) => $fiber->resume($bytes));
    }, $loop);
});

$loop->defer(fn() => $fiber->start());

echo "Waiting for data...\n";

// Read data in main fiber.
$data = \Fiber::suspend(function (Fiber $fiber) use ($loop, $read): void {
    $loop->read($read, fn(?string $data) => $fiber->resume($data));
}, $loop);

echo "Received data: ", $data, "\n";
