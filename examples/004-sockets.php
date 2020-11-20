<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

[$read, $write] = createSocketPair();

$loop = new Loop;

// Write data in a separate fiber after a 1 second delay.
$fiber = Fiber::create(function () use ($loop, $write): void {
    \Fiber::suspend(function (Continuation $continuation) use ($loop): void {
        $loop->delay(1000, fn() => $continuation->resume());
    }, $loop);

    \Fiber::suspend(function (Continuation $continuation) use ($loop, $write): void {
        $loop->write($write, 'Hello, world!', fn(int $bytes) => $continuation->resume($bytes));
    }, $loop);
});

$loop->defer(fn() => $fiber->run());

echo "Waiting for data...\n";

// Read data in main fiber.
$data = \Fiber::suspend(function (Continuation $continuation) use ($loop, $read): void {
    $loop->read($read, fn(?string $data) => $continuation->resume($data));
}, $loop);

echo "Received data: ", $data, "\n";
