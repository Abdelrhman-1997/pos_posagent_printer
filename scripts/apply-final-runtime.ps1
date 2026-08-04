$ErrorActionPreference = 'Stop'

$sourceDir = Join-Path $PSScriptRoot '..\agent-src'
$overrideDir = Join-Path $PSScriptRoot '..\agent-overrides'

$files = @(
    'dsoftsingleinstance.h', 'dsoftsingleinstance.cpp',
    'dsoftlegacymigrator.h', 'dsoftlegacymigrator.cpp',
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
    Set-Content $cmakePath $cmake -NoNewline -Encoding utf8
}

$mainPath = Join-Path $sourceDir 'main.cpp'
$main = Get-Content $mainPath -Raw
if ($main -notmatch '#include "dsoftsingleinstance.h"') {
    $main = $main.Replace('#include "messagesystem.h"', "#include `"messagesystem.h`"`r`n#include `"dsoftsingleinstance.h`"`r`n#include <QMessageBox>")
}

# Replace the fragile process-name comparison with a lock file after QApplication exists.
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

if (-not (Select-String $cmakePath -SimpleMatch 'dsoftlegacymigrator.cpp' -Quiet)) {
    throw 'Final runtime files were not added to CMake.'
}
if (-not (Select-String $mainPath -SimpleMatch 'DSoftSingleInstance singleInstance' -Quiet)) {
    throw 'Single-instance guard was not installed in main.cpp.'
}

Write-Host 'Applied DSoft single-instance guard and legacy migration.'
