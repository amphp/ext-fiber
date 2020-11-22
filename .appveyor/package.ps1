$ErrorActionPreference = "Stop"

if ($env:TS -eq '0') {
    $ts_part = 'nts'
} else {
    $ts_part = 'ts';
}

if ($env:APPVEYOR_REPO_TAG -eq "true") {
    $bname = "php_fiber-$env:APPVEYOR_REPO_TAG_NAME-$env:PHP_VER-$ts_part-$env:VC-$env:ARCH"
} else {
    $bname = "php_fiber-$($env:APPVEYOR_REPO_COMMIT.substring(0, 8))-$env:PHP_VER-$ts_part-$env:VC-$env:ARCH"
}
$zip_bname = "$bname.zip"

$dir = 'C:\projects\ext-fiber\';
if ($env:ARCH -eq 'x64') {
    $dir += 'x64\'
}
$dir += 'Release'
if ($env:TS -eq '1') {
    $dir += '_TS'
}

Compress-Archive "$dir\php_fiber.dll", "$dir\php_fiber.pdb", "C:\projects\ext-fiber\LICENSE" "C:\$zip_bname"
Push-AppveyorArtifact "C:\$zip_bname"
