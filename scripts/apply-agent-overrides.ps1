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
    'printerprofiledialog.h',
    'printerprofiledialog.cpp',
    'printermanagerwidget.h',
    'printermanagerwidget.cpp',
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
        printerprofiledialog.h printerprofiledialog.cpp
        printermanagerwidget.h printermanagerwidget.cpp
'@

if ($cmake -notmatch [regex]::Escape('printermanagerwidget.cpp')) {
    if ($cmake -match [regex]::Escape('dsoftruntime.cpp')) {
        $cmake = [regex]::Replace(
            $cmake,
            'dsoftruntime\.h\s+dsoftruntime\.cpp',
            "dsoftruntime.h dsoftruntime.cpp`r`n        printerprofiledialog.h printerprofiledialog.cpp`r`n        printermanagerwidget.h printermanagerwidget.cpp",
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

$mainWindowPath = Join-Path $sourceDir 'mainwindow.cpp'
$mainWindow = Get-Content $mainWindowPath -Raw

if ($mainWindow -notmatch '#include "dsoftruntime.h"') {
    $includeAnchor = '#include "messagesystem.h"'
    if ($mainWindow -notmatch [regex]::Escape($includeAnchor)) {
        throw 'Could not locate messagesystem include in mainwindow.cpp.'
    }
    $mainWindow = $mainWindow.Replace(
        $includeAnchor,
        "$includeAnchor`r`n#include `"dsoftruntime.h`"`r`n#include `"printermanagerwidget.h`"`r`n#include <QDockWidget>"
    )
} elseif ($mainWindow -notmatch '#include "printermanagerwidget.h"') {
    $mainWindow = $mainWindow.Replace(
        '#include "dsoftruntime.h"',
        "#include `"dsoftruntime.h`"`r`n#include `"printermanagerwidget.h`"`r`n#include <QDockWidget>"
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
  DSoftJobResult dsoftResult;
  while (DSoftRuntime::instance().processNext(&dsoftResult))
    processedDSoftJob = true;

  if (GlobalState::processQueue() || processedDSoftJob) {
    if (showpreview)
      scheduleDiplayPreviewUpdate();
  }

  if (GlobalState::getPrinterStatus() == CONNECTED) {
'@

if ($mainWindow -match [regex]::Escape('DSoftRuntime::instance().processNextJob()')) {
    $mainWindow = $mainWindow.Replace(
        'while (DSoftRuntime::instance().processNextJob())',
        'DSoftJobResult dsoftResult;`r`n  while (DSoftRuntime::instance().processNext(&dsoftResult))'
    )
} elseif ($mainWindow -notmatch [regex]::Escape('DSoftRuntime::instance().processNext(&dsoftResult)')) {
    if ($mainWindow -notmatch [regex]::Escape($oldRefresh.Trim())) {
        throw 'Could not locate refreshTimer implementation in mainwindow.cpp.'
    }
    $mainWindow = $mainWindow.Replace($oldRefresh.Trim(), $newRefresh.Trim())
}

$mainWindow = $mainWindow.Replace('t->setInterval(100);', 't->setInterval(1000);')

# Add the professional printer-management panel without changing the upstream .ui file.
$uiAnchor = 'ui->setupUi(this);'
if ($mainWindow -notmatch [regex]::Escape('DSoft Printer Management')) {
    if ($mainWindow -notmatch [regex]::Escape($uiAnchor)) {
        throw 'Could not locate setupUi call in mainwindow.cpp.'
    }
    $panelCode = @'
ui->setupUi(this);
  setWindowTitle(QStringLiteral("DSoft POS Printer Agent"));

  auto *printerDock = new QDockWidget(QStringLiteral("DSoft Printer Management"), this);
  printerDock->setObjectName(QStringLiteral("dsoft_printer_management_dock"));
  printerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  printerDock->setWidget(new PrinterManagerWidget(printerDock));
  addDockWidget(Qt::RightDockWidgetArea, printerDock);
'@
    $mainWindow = $mainWindow.Replace($uiAnchor, $panelCode.TrimEnd())
}

Set-Content -Path $mainWindowPath -Value $mainWindow -NoNewline -Encoding utf8

foreach ($file in $overrideFiles) {
    if (-not (Test-Path (Join-Path $sourceDir $file))) {
        throw "Override was not copied into agent-src: $file"
    }
}

if (-not (Select-String -Path $cmakePath -SimpleMatch 'printermanagerwidget.cpp' -Quiet)) {
    throw 'CMakeLists.txt was not updated with the printer management UI.'
}
if (-not (Select-String -Path $mainWindowPath -SimpleMatch 'DSoftRuntime::instance().processNext(&dsoftResult)' -Quiet)) {
    throw 'mainwindow.cpp was not connected to the DSoft runtime.'
}
if (-not (Select-String -Path $mainWindowPath -SimpleMatch 'DSoft Printer Management' -Quiet)) {
    throw 'mainwindow.cpp was not connected to the printer manager widget.'
}
if (-not (Select-String -Path (Join-Path $sourceDir 'json.cpp') -SimpleMatch 'enqueueReceipt' -Quiet)) {
    throw 'json.cpp was not replaced with the routed implementation.'
}

Write-Host 'Applied DSoft routed printing and printer-management UI.'
Write-Host ('Integrated files: ' + ($overrideFiles -join ', '))
