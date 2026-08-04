$ErrorActionPreference = 'Stop'

$sourceDir = Join-Path $PSScriptRoot '..\agent-src'
$overrideDir = Join-Path $PSScriptRoot '..\agent-overrides'

if (-not (Test-Path $sourceDir)) {
    throw "POSAgent source directory was not found: $sourceDir"
}
if (-not (Test-Path $overrideDir)) {
    throw "Override directory was not found: $overrideDir"
}

$overrideFiles = @(
    'printerprofile.h',
    'printerprofile.cpp',
    'printerprofilemanager.h',
    'printerprofilemanager.cpp',
    'printerroutingrequest.h',
    'printerroutingrequest.cpp',
    'dsoftprintjob.h',
    'dsoftprintqueue.h',
    'dsoftprintqueue.cpp'
)

foreach ($file in $overrideFiles) {
    $source = Join-Path $overrideDir $file
    $target = Join-Path $sourceDir $file
    if (-not (Test-Path $source)) {
        throw "Required override file is missing: $source"
    }
    Copy-Item -Path $source -Destination $target -Force
}

$cmakePath = Join-Path $sourceDir 'CMakeLists.txt'
$cmake = Get-Content $cmakePath -Raw
$anchor = '        eventsystem.h printagent.qrc'
$replacement = @'
        eventsystem.h printagent.qrc
        printerprofile.h printerprofile.cpp
        printerprofilemanager.h printerprofilemanager.cpp
        printerroutingrequest.h printerroutingrequest.cpp
        dsoftprintjob.h dsoftprintqueue.h dsoftprintqueue.cpp
'@

if ($cmake -notmatch [regex]::Escape('dsoftprintqueue.cpp')) {
    if ($cmake -match [regex]::Escape('printerprofilemanager.cpp')) {
        $cmake = [regex]::Replace(
            $cmake,
            'printerroutingrequest\.h\s+printerroutingrequest\.cpp',
            "printerroutingrequest.h printerroutingrequest.cpp`r`n        dsoftprintjob.h dsoftprintqueue.h dsoftprintqueue.cpp",
            1
        )
    } else {
        if ($cmake -notmatch [regex]::Escape($anchor)) {
            throw 'Could not locate PROJECT_SOURCES anchor in CMakeLists.txt.'
        }
        $cmake = $cmake.Replace($anchor, $replacement.TrimEnd())
    }
    Set-Content -Path $cmakePath -Value $cmake -NoNewline -Encoding utf8
}

foreach ($file in $overrideFiles) {
    if (-not (Test-Path (Join-Path $sourceDir $file))) {
        throw "Override was not copied into agent-src: $file"
    }
}

if (-not (Select-String -Path $cmakePath -SimpleMatch 'dsoftprintqueue.cpp' -Quiet)) {
    throw 'CMakeLists.txt was not updated with DSoft routed queue sources.'
}

Write-Host 'Applied DSoft multi-printer foundation sources.'
Write-Host ('Integrated files: ' + ($overrideFiles -join ', '))
