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
    'dsoftprintqueue.cpp',
    'dsoftprinterservice.h',
    'dsoftprinterservice.cpp',
    'dsoftwindowsprinterdispatcher.h',
    'dsoftwindowsprinterdispatcher.cpp',
    'dsoftruntime.h',
    'dsoftruntime.cpp',
    'json.cpp'
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
        dsoftprinterservice.h dsoftprinterservice.cpp
        dsoftwindowsprinterdispatcher.h dsoftwindowsprinterdispatcher.cpp
        dsoftruntime.h dsoftruntime.cpp
'@

if ($cmake -notmatch [regex]::Escape('dsoftruntime.cpp')) {
    if ($cmake -match [regex]::Escape('dsoftwindowsprinterdispatcher.cpp')) {
        $cmake = [regex]::Replace(
            $cmake,
            'dsoftwindowsprinterdispatcher\.h\s+dsoftwindowsprinterdispatcher\.cpp',
            "dsoftwindowsprinterdispatcher.h dsoftwindowsprinterdispatcher.cpp`r`n        dsoftruntime.h dsoftruntime.cpp",
            1
        )
    } elseif ($cmake -match [regex]::Escape('dsoftprinterservice.cpp')) {
        $cmake = [regex]::Replace(
            $cmake,
            'dsoftprinterservice\.h\s+dsoftprinterservice\.cpp',
            "dsoftprinterservice.h dsoftprinterservice.cpp`r`n        dsoftwindowsprinterdispatcher.h dsoftwindowsprinterdispatcher.cpp`r`n        dsoftruntime.h dsoftruntime.cpp",
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

# Process the routed queue immediately when a network request wakes the UI.
$mainWindowPath = Join-Path $sourceDir 'mainwindow.cpp'
$mainWindow = Get-Content $mainWindowPath -Raw
if ($mainWindow -notmatch '#include "dsoftruntime.h"') {
    $includeAnchor = '#include "messagesystem.h"'
    if ($mainWindow -notmatch [regex]::Escape($includeAnchor)) {
        throw 'Could not locate messagesystem include in mainwindow.cpp.'
    }
    $mainWindow = $mainWindow.Replace(
        $includeAnchor,
        "$includeAnchor`r`n#include `"dsoftruntime.h`""
    )
}

$oldRefresh = @'
void MainWindow::refreshTimer() {
  if (GlobalState::processQueue()) {
    if (showpreview)
      scheduleDiplayPreviewUpdate();
  }

  if (GlobalState::getPrinterStatus() == CONNECTED) {
'@
$newRefresh = @'
void MainWindow::refreshTimer() {
  bool processedDSoftJob = false;
  while (DSoftRuntime::instance().processNextJob())
    processedDSoftJob = true;

  if (GlobalState::processQueue() || processedDSoftJob) {
    if (showpreview)
      scheduleDiplayPreviewUpdate();
  }

  if (GlobalState::getPrinterStatus() == CONNECTED) {
'@
if ($mainWindow -notmatch [regex]::Escape('DSoftRuntime::instance().processNextJob()')) {
    if ($mainWindow -notmatch [regex]::Escape($oldRefresh.Trim())) {
        throw 'Could not locate refreshTimer implementation in mainwindow.cpp.'
    }
    $mainWindow = $mainWindow.Replace($oldRefresh.Trim(), $newRefresh.Trim())
}

# Immediate invokeMethod is the normal path; keep a slower fallback timer.
$mainWindow = $mainWindow.Replace('t->setInterval(100);', 't->setInterval(1000);')
Set-Content -Path $mainWindowPath -Value $mainWindow -NoNewline -Encoding utf8

foreach ($file in $overrideFiles) {
    if (-not (Test-Path (Join-Path $sourceDir $file))) {
        throw "Override was not copied into agent-src: $file"
    }
}

if (-not (Select-String -Path $cmakePath -SimpleMatch 'dsoftruntime.cpp' -Quiet)) {
    throw 'CMakeLists.txt was not updated with all DSoft sources.'
}
if (-not (Select-String -Path $mainWindowPath -SimpleMatch 'DSoftRuntime::instance().processNextJob()' -Quiet)) {
    throw 'mainwindow.cpp was not connected to the DSoft runtime.'
}
if (-not (Select-String -Path (Join-Path $sourceDir 'json.cpp') -SimpleMatch 'enqueueReceipt' -Quiet)) {
    throw 'json.cpp was not replaced with the routed implementation.'
}

Write-Host 'Applied DSoft routed JSON and immediate queue processing.'
Write-Host ('Integrated files: ' + ($overrideFiles -join ', '))
