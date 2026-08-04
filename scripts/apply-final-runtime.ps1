$ErrorActionPreference = 'Stop'

$sourceDir = Join-Path $PSScriptRoot '..\agent-src'

if (-not (Test-Path $sourceDir)) {
    throw "POSAgent source directory was not found: $sourceDir"
}

$cmakePath = Join-Path $sourceDir 'CMakeLists.txt'
$mainPath = Join-Path $sourceDir 'main.cpp'
$mainWindowPath = Join-Path $sourceDir 'mainwindow.cpp'

foreach ($path in @($cmakePath, $mainPath, $mainWindowPath)) {
    if (-not (Test-Path $path)) {
        throw "Required source file was not found: $path"
    }
}

$cmake = Get-Content $cmakePath -Raw
$main = Get-Content $mainPath -Raw
$mainWindow = Get-Content $mainWindowPath -Raw

$forbiddenCMakeEntries = @(
    'printerprofilemanager.cpp',
    'dsoftprintqueue.cpp',
    'dsoftprinterservice.cpp',
    'printermanagerwidget.cpp',
    'dsoftoperationswidget.cpp',
    'dsoftlogindialog.cpp'
)

foreach ($entry in $forbiddenCMakeEntries) {
    if ($cmake -match [regex]::Escape($entry)) {
        throw "Complex runtime source is still enabled in CMakeLists.txt: $entry"
    }
}

$forbiddenMainEntries = @(
    'DSoftLoginDialog',
    'DSoftSingleInstance',
    '/dsoft/api/v1/printers',
    '/dsoft/api/v1/health'
)

foreach ($entry in $forbiddenMainEntries) {
    if ($main -match [regex]::Escape($entry)) {
        throw "Complex runtime feature is still enabled in main.cpp: $entry"
    }
}

$forbiddenWindowEntries = @(
    'DSoft Printer Management',
    'DSoft Operations',
    'PrinterManagerWidget',
    'DSoftOperationsWidget'
)

foreach ($entry in $forbiddenWindowEntries) {
    if ($mainWindow -match [regex]::Escape($entry)) {
        throw "Complex UI feature is still enabled in mainwindow.cpp: $entry"
    }
}

Write-Host 'Confirmed minimal single-printer runtime.'
Write-Host 'The original POSAgent print endpoint remains available on port 9069.'
