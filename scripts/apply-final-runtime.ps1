$ErrorActionPreference = 'Stop'

$sourceDir = Join-Path $PSScriptRoot '..\agent-src'
$overrideDir = Join-Path $PSScriptRoot '..\agent-overrides'

$files = @(
    'dsoftsingleinstance.h', 'dsoftsingleinstance.cpp',
    'dsoftlegacymigrator.h', 'dsoftlegacymigrator.cpp',
    'dsoftoperationswidget.h', 'dsoftoperationswidget.cpp',
    'dsoftruntime.cpp'
)

foreach ($file in $files) {
    $source = Join-Path $overrideDir $file
    if (-not (Test-Path $source)) { throw "Missing final runtime file: $source" }
    Copy-Item $source (Join-Path $sourceDir $file) -Force
}

$cmakePath = Join-Path $sourceDir 'CMakeLists.txt'
$cmake = Get-Content $cmakePath -Raw
if ($cmake -notmatch 'dsoftsingleinstance\.cpp') {
    $anchor = 'dsoftstartupmanager.h dsoftstartupmanager.cpp'
    if ($cmake -notmatch [regex]::Escape($anchor)) {
        throw 'Could not locate DSoft startup source anchor in CMakeLists.txt.'
    }
    $replacement = "$anchor`r`n        dsoftsingleinstance.h dsoftsingleinstance.cpp`r`n        dsoftlegacymigrator.h dsoftlegacymigrator.cpp"
    $cmake = $cmake.Replace($anchor, $replacement)
}
if ($cmake -notmatch 'dsoftoperationswidget\.cpp') {
    $anchor = 'dsoftlegacymigrator.h dsoftlegacymigrator.cpp'
    if ($cmake -notmatch [regex]::Escape($anchor)) {
        throw 'Could not locate legacy migrator source anchor in CMakeLists.txt.'
    }
    $cmake = $cmake.Replace($anchor, "$anchor`r`n        dsoftoperationswidget.h dsoftoperationswidget.cpp")
}
Set-Content $cmakePath $cmake -NoNewline -Encoding utf8

$mainPath = Join-Path $sourceDir 'main.cpp'
$main = Get-Content $mainPath -Raw
if ($main -notmatch '#include "dsoftsingleinstance.h"') {
    $main = $main.Replace('#include "messagesystem.h"', "#include `"messagesystem.h`"`r`n#include `"dsoftsingleinstance.h`"`r`n#include <QMessageBox>")
}

$oldApp = @'
  QApplication a(argc, argv);
  MainWindow w;
'@
$newApp = @'
  QApplication a(argc, argv);
  a.setOrganizationName(QStringLiteral("DSoft"));
  a.setApplicationName(QStringLiteral("DSoft POS Printer Agent"));

  DSoftSingleInstance singleInstance(QStringLiteral("dsoft-pos-printer-agent"));
  QString singleInstanceError;
  if (!singleInstance.tryAcquire(&singleInstanceError)) {
    QMessageBox::information(nullptr, QStringLiteral("DSoft POS Printer Agent"),
                             singleInstanceError);
    return 0;
  }

  MainWindow w;
'@
if ($main -notmatch 'DSoftSingleInstance singleInstance') {
    if ($main -notmatch [regex]::Escape($oldApp.Trim())) {
        throw 'Could not locate QApplication/MainWindow block in main.cpp.'
    }
    $main = $main.Replace($oldApp.Trim(), $newApp.Trim())
}
Set-Content $mainPath $main -NoNewline -Encoding utf8

$mainWindowPath = Join-Path $sourceDir 'mainwindow.cpp'
$mainWindow = Get-Content $mainWindowPath -Raw
if ($mainWindow -notmatch '#include "dsoftoperationswidget.h"') {
    $anchor = '#include "printermanagerwidget.h"'
    if ($mainWindow -notmatch [regex]::Escape($anchor)) {
        throw 'Could not locate printer manager include in mainwindow.cpp.'
    }
    $mainWindow = $mainWindow.Replace($anchor, "$anchor`r`n#include `"dsoftoperationswidget.h`"")
}
if ($mainWindow -notmatch 'DSoft Operations') {
    $anchor = 'addDockWidget(Qt::RightDockWidgetArea, printerDock);'
    if ($mainWindow -notmatch [regex]::Escape($anchor)) {
        throw 'Could not locate printer management dock in mainwindow.cpp.'
    }
    $operationsDock = @'
addDockWidget(Qt::RightDockWidgetArea, printerDock);

  auto *operationsDock = new QDockWidget(QStringLiteral("DSoft Operations"), this);
  operationsDock->setObjectName(QStringLiteral("dsoft_operations_dock"));
  operationsDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
  operationsDock->setWidget(new DSoftOperationsWidget(operationsDock));
  addDockWidget(Qt::BottomDockWidgetArea, operationsDock);
'@
    $mainWindow = $mainWindow.Replace($anchor, $operationsDock.TrimEnd())
}
Set-Content $mainWindowPath $mainWindow -NoNewline -Encoding utf8

if (-not (Select-String $cmakePath -SimpleMatch 'dsoftoperationswidget.cpp' -Quiet)) {
    throw 'Operations widget files were not added to CMake.'
}
if (-not (Select-String $mainPath -SimpleMatch 'DSoftSingleInstance singleInstance' -Quiet)) {
    throw 'Single-instance guard was not installed in main.cpp.'
}
if (-not (Select-String $mainWindowPath -SimpleMatch 'DSoft Operations' -Quiet)) {
    throw 'Operations dashboard was not installed in mainwindow.cpp.'
}

Write-Host 'Applied DSoft single-instance guard, migration, and operations dashboard.'
