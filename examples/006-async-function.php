<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

// Create three new fibers using the async() function.
async($loop, function () use ($loop): void {
    delay($loop, 1500);
    var_dump(1);
});

async($loop, function () use ($loop): void {
    delay($loop, 1000);
    var_dump(2);
});

async($loop, function () use ($loop): void {
    delay($loop, 2000);
    var_dump(3);
});

// Suspend the main thread to enter the FiberScheduler.
delay($loop, 500);
var_dump(4);
