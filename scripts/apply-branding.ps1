$ErrorActionPreference = 'Stop'

$sourceDir = Join-Path $PSScriptRoot '..\agent-src'
$overrideDir = Join-Path $PSScriptRoot '..\agent-overrides'

$resourceSource = Join-Path $overrideDir 'posagent_resource.rc'
$resourceTarget = Join-Path $sourceDir 'posagent_resource.rc'

if (-not (Test-Path $resourceSource)) {
    throw "Branding resource was not found: $resourceSource"
}

Copy-Item $resourceSource $resourceTarget -Force

$mainPath = Join-Path $sourceDir 'main.cpp'
$main = Get-Content $mainPath -Raw

if ($main -notmatch 'setApplicationDisplayName') {
    $anchor = 'a.setApplicationName(QStringLiteral("DSoft POS Printer Agent"));'
    if ($main -notmatch [regex]::Escape($anchor)) {
        throw 'Could not locate DSoft application name in main.cpp.'
    }
    $replacement = @'
a.setApplicationName(QStringLiteral("DSoft POS Printer Agent"));
  a.setApplicationDisplayName(QStringLiteral("DSoft POS Printer Agent"));
  a.setApplicationVersion(QStringLiteral("1.0.0"));
'@
    $main = $main.Replace($anchor, $replacement.TrimEnd())
    Set-Content $mainPath $main -NoNewline -Encoding utf8
}

if (-not (Select-String $resourceTarget -SimpleMatch 'DSoft POS Printer Agent' -Quiet)) {
    throw 'Windows resource branding was not applied.'
}
if (-not (Select-String $mainPath -SimpleMatch 'setApplicationVersion(QStringLiteral("1.0.0"))' -Quiet)) {
    throw 'Qt application branding was not applied.'
}

Write-Host 'Applied DSoft Windows and Qt branding.'
