$ErrorActionPreference = 'Stop'

$sourceDir = Join-Path $PSScriptRoot '..\agent-src'
$overrideDir = Join-Path $PSScriptRoot '..\agent-overrides'

$files = @(
    'dsoftsingleinstance.h', 'dsoftsingleinstance.cpp',
    'dsoftlegacymigrator.h', 'dsoftlegacymigrator.cpp',
    'dsoftoperationswidget.h', 'dsoftoperationswidget.cpp',
    'dsoftstatusapi.h', 'dsoftstatusapi.cpp',
    'dsoftlogindialog.h', 'dsoftlogindialog.cpp',
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
if ($cmake -notmatch 'dsoftstatusapi\.cpp') {
    $anchor = 'dsoftoperationswidget.h dsoftoperationswidget.cpp'
    if ($cmake -notmatch [regex]::Escape($anchor)) {
        throw 'Could not locate operations widget source anchor in CMakeLists.txt.'
    }
    $cmake = $cmake.Replace($anchor, "$anchor`r`n        dsoftstatusapi.h dsoftstatusapi.cpp")
}
if ($cmake -notmatch 'dsoftlogindialog\.cpp') {
    $anchor = 'dsoftstatusapi.h dsoftstatusapi.cpp'
    if ($cmake -notmatch [regex]::Escape($anchor)) {
        throw 'Could not locate status API source anchor in CMakeLists.txt.'
    }
    $cmake = $cmake.Replace($anchor, "$anchor`r`n        dsoftlogindialog.h dsoftlogindialog.cpp")
}
Set-Content $cmakePath $cmake -NoNewline -Encoding utf8

$mainPath = Join-Path $sourceDir 'main.cpp'
$main = Get-Content $mainPath -Raw
if ($main -notmatch '#include "dsoftsingleinstance.h"') {
    $main = $main.Replace('#include "messagesystem.h"', "#include `"messagesystem.h`"`r`n#include `"dsoftsingleinstance.h`"`r`n#include `"dsoftstatusapi.h`"`r`n#include `"dsoftlogindialog.h`"`r`n#include <QMessageBox>")
} else {
    if ($main -notmatch '#include "dsoftstatusapi.h"') {
        $main = $main.Replace('#include "dsoftsingleinstance.h"', "#include `"dsoftsingleinstance.h`"`r`n#include `"dsoftstatusapi.h`"")
    }
    if ($main -notmatch '#include "dsoftlogindialog.h"') {
        $main = $main.Replace('#include "dsoftstatusapi.h"', "#include `"dsoftstatusapi.h`"`r`n#include `"dsoftlogindialog.h`"")
    }
}

if ($main -notmatch '/dsoft/api/v1/printers') {
    $routeAnchor = '.post("/hw_proxy/handshake",'
    if ($main -notmatch [regex]::Escape($routeAnchor)) {
        throw 'Could not locate handshake route in main.cpp.'
    }
    $routes = @'
.get("/dsoft/api/v1/health",
           [](auto *res, auto *) {
             res->writeHeader("Content-Type", "application/json; charset=utf-8");
             res->writeHeader("Access-Control-Allow-Origin", "*");
             res->writeHeader("Cache-Control", "no-store");
             res->writeHeader("Connection", "close");
             res->end(DSoftStatusApi::healthJson());
           })
      .get("/dsoft/api/v1/printers",
           [](auto *res, auto *) {
             res->writeHeader("Content-Type", "application/json; charset=utf-8");
             res->writeHeader("Access-Control-Allow-Origin", "*");
             res->writeHeader("Cache-Control", "no-store");
             res->writeHeader("Connection", "close");
             res->end(DSoftStatusApi::printersJson());
           })
      .post("/hw_proxy/handshake",
'@
    $main = $main.Replace($routeAnchor, $routes.TrimEnd())
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

  DSoftLoginDialog loginDialog;
  if (loginDialog.exec() != QDialog::Accepted)
    return 0;

  MainWindow w;
'@
if ($main -notmatch 'DSoftSingleInstance singleInstance') {
    if ($main -notmatch [regex]::Escape($oldApp.Trim())) {
        throw 'Could not locate QApplication/MainWindow block in main.cpp.'
    }
    $main = $main.Replace($oldApp.Trim(), $newApp.Trim())
} elseif ($main -notmatch 'DSoftLoginDialog loginDialog') {
    $anchor = '  MainWindow w;'
    if ($main -notmatch [regex]::Escape($anchor)) {
        throw 'Could not locate MainWindow construction in main.cpp.'
    }
    $loginBlock = @'
  DSoftLoginDialog loginDialog;
  if (loginDialog.exec() != QDialog::Accepted)
    return 0;

  MainWindow w;
'@
    $main = $main.Replace($anchor, $loginBlock.TrimEnd())
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

if (-not (Select-String -Path $cmakePath -SimpleMatch 'dsoftoperationswidget.cpp' -Quiet)) {
    throw 'Operations widget files were not added to CMake.'
}
if (-not (Select-String -Path $cmakePath -SimpleMatch 'dsoftstatusapi.cpp' -Quiet)) {
    throw 'Status API files were not added to CMake.'
}
if (-not (Select-String -Path $cmakePath -SimpleMatch 'dsoftlogindialog.cpp' -Quiet)) {
    throw 'Login dialog files were not added to CMake.'
}
if (-not (Select-String -Path $mainPath -SimpleMatch 'DSoftSingleInstance singleInstance' -Quiet)) {
    throw 'Single-instance guard was not installed in main.cpp.'
}
if (-not (Select-String -Path $mainPath -SimpleMatch 'DSoftLoginDialog loginDialog' -Quiet)) {
    throw 'Admin login was not installed in main.cpp.'
}
if (-not (Select-String -Path $mainPath -SimpleMatch '/dsoft/api/v1/printers' -Quiet)) {
    throw 'DSoft status API routes were not installed in main.cpp.'
}
if (-not (Select-String -Path $mainWindowPath -SimpleMatch 'DSoft Operations' -Quiet)) {
    throw 'Operations dashboard was not installed in mainwindow.cpp.'
}

Write-Host 'Applied DSoft single-instance guard, fixed admin login, migration, operations dashboard, and status API.'
