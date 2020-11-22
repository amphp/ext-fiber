$ErrorActionPreference = "Stop"

$dir = 'C:\projects\ext-fiber\'
if ($env:ARCH -eq 'x64') {
    $dir += 'x64\'
}
$dir += 'Release'
if ($env:TS -eq '1') {
    $dir += '_TS'
}

$fiber_dll_opt = "-d extension=$dir\php_fiber.dll"

Set-Location "C:\projects\ext-fiber"

$php = Get-Command 'php' | Select-Object -ExpandProperty 'Definition'
$dname = (Get-Item $php).Directory.FullName

$opts = '-n'
if ($env:OPCACHE -ne '0') {
    $opts += " -d zend_extension=$dname\ext\php_opcache.dll -d opcache.enabled=1 -d opcache.enable_cli=1 -d opcache.optimization_level=0"
}
$opts += " $fiber_dll_opt"
$env:TEST_PHP_ARGS = $opts

$env:TEST_PHP_EXECUTABLE = $php
& $php run-tests.php -q --show-diff tests
if (-not $?) {
    throw "tests failed with errorlevel $LastExitCode"
}
