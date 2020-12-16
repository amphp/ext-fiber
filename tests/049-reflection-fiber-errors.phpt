--TEST--
ReflectionFiber errors
--SKIPIF--
<?php include __DIR__ . '/include/skip-if.php';
--FILE--
<?php

require dirname(__DIR__) . '/scripts/bootstrap.php';

$loop = new Loop;

$fiber = new Fiber(function () use ($loop): void {
    $fiber = Fiber::this();
    $loop->defer(fn() => $fiber->resume());
    Fiber::suspend($loop);
});

$reflection = new ReflectionFiber($fiber);

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

$fiber = Fiber::this();
$loop->delay(10, fn() => $fiber->resume());
Fiber::suspend($loop);

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
