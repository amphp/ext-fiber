<?php

spl_autoload_register(function (string $className): void {
    require __DIR__ . '/' . $className . '.php';
});

require __DIR__ . '/functions.php';
