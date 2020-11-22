--TEST--
ReflectionFiber status methods
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = Fiber::create(function () use ($loop): void {
    Fiber::suspend(function (Fiber $fiber) use ($loop): void {
        $loop->defer(fn() => $fiber->resume());
    }, $loop);
});

$reflection = ReflectionFiber::fromFiber($fiber);

try {
    $reflection->getTrace();
} catch (Error $error) {
    echo $error->getMessage(), "\n";
}

try {
    $reflection->getExecutingFile();
} catch (Error $error) {
    echo $error->getMessage(), "\n";
}

try {
    $reflection->getExecutingLine();
} catch (Error $error) {
    echo $error->getMessage(), "\n";
}

$loop->defer(fn() => $fiber->start());

$loop->defer(fn() => var_dump($reflection->getExecutingFile()));
$loop->defer(fn() => var_dump($reflection->getExecutingLine()));

Fiber::suspend(function (Fiber $fiber) use ($loop): void {
    $loop->delay(10, fn() => $fiber->resume());
}, $loop);

try {
    $reflection->getTrace();
} catch (Error $error) {
    echo $error->getMessage(), "\n";
}

try {
    $reflection->getExecutingFile();
} catch (Error $error) {
    echo $error->getMessage(), "\n";
}

try {
    $reflection->getExecutingLine();
} catch (Error $error) {
    echo $error->getMessage(), "\n";
}

--EXPECTF--
Cannot fetch information from a fiber that has not been started or is terminated
Cannot fetch information from a fiber that has not been started or is terminated
Cannot fetch information from a fiber that has not been started or is terminated
string(%d) "%s%e049-reflection-fiber-errors.php"
int(10)
Cannot fetch information from a fiber that has not been started or is terminated
Cannot fetch information from a fiber that has not been started or is terminated
Cannot fetch information from a fiber that has not been started or is terminated
