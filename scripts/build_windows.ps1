<#
Build the pinned upstream POSAgent source as a portable Windows x64 package.
This script does not modify Odoo core or the Odoo addon.
#>

param(
    [string]$QtDir = "C:\Qt\6.5.2\msvc2019_64",
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$CommitFile = Join-Path $Root "UPSTREAM_COMMIT"
$SourceDir = Join-Path $Root ".cache\posagent-src"
$BuildDir = Join-Path $Root "build-win64"
$PackageDir = Join-Path $Root "package\posagent-win64-dev"
$ReleaseDir = Join-Path $Root "releases"

if (-not (Test-Path $CommitFile)) {
    throw "Missing UPSTREAM_COMMIT"
}
if (-not (Test-Path $QtDir)) {
    throw "Qt kit not found: $QtDir"
}

$UpstreamCommit = (Get-Content $CommitFile -Raw).Trim()
if ($UpstreamCommit -notmatch '^[0-9a-f]{40}$') {
    throw "UPSTREAM_COMMIT must contain one full 40-character Git SHA"
}

if (-not (Test-Path (Join-Path $SourceDir ".git"))) {
    New-Item -ItemType Directory -Force -Path $SourceDir | Out-Null
    git -C $SourceDir init
    git -C $SourceDir remote add origin https://github.com/dieg0-a/posagentpro-src.git
}

git -C $SourceDir fetch --depth 1 origin $UpstreamCommit
git -C $SourceDir checkout --detach --force FETCH_HEAD
git -C $SourceDir clean -ffd

# Upstream hard-codes the default Qt path. Keep that baseline when possible;
# for a custom local Qt path, adjust only the build-system path in the fetched copy.
if ($QtDir -ne "C:\Qt\6.5.2\msvc2019_64") {
    $cmakePath = Join-Path $SourceDir "CMakeLists.txt"
    $cmake = Get-Content $cmakePath -Raw
    $escaped = $QtDir.Replace('\', '\\')
    $cmake = $cmake.Replace('set (CMAKE_PREFIX_PATH "C:\\Qt\\6.5.2\\msvc2019_64")', "set (CMAKE_PREFIX_PATH `"$escaped`")")
    Set-Content -Path $cmakePath -Value $cmake -Encoding utf8
}

Remove-Item -Recurse -Force $BuildDir, $PackageDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $BuildDir, $PackageDir, $ReleaseDir | Out-Null

cmake -S $SourceDir -B $BuildDir -A x64 -DCMAKE_BUILD_TYPE=$Configuration
cmake --build $BuildDir --config $Configuration

$BuiltExe = Get-ChildItem -Path $BuildDir -Recurse -Filter posagent.exe | Select-Object -First 1
if (-not $BuiltExe) {
    throw "posagent.exe was not produced"
}

$DevExe = Join-Path $PackageDir "posagent-win64-dev.exe"
Copy-Item $BuiltExe.FullName $DevExe -Force

$DeployTool = Join-Path $QtDir "bin\windeployqt.exe"
if (-not (Test-Path $DeployTool)) {
    throw "windeployqt.exe not found: $DeployTool"
}
& $DeployTool --release --compiler-runtime $DevExe

$Required = @(
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll",
    "Qt6PrintSupport.dll",
    "platforms\qwindows.dll"
)
foreach ($RelativePath in $Required) {
    if (-not (Test-Path (Join-Path $PackageDir $RelativePath))) {
        throw "Missing deployed runtime file: $RelativePath"
    }
}

Copy-Item (Join-Path $SourceDir "LICENSE") (Join-Path $PackageDir "LICENSE-posagent.txt") -Force
$NoticesDir = Join-Path $PackageDir "THIRD-PARTY-NOTICES"
New-Item -ItemType Directory -Force -Path $NoticesDir | Out-Null
if (Test-Path (Join-Path $SourceDir "zlib\LICENSE")) {
    Copy-Item (Join-Path $SourceDir "zlib\LICENSE") (Join-Path $NoticesDir "zlib-LICENSE.txt")
}
if (Test-Path (Join-Path $SourceDir "rapidjson\license.txt")) {
    Copy-Item (Join-Path $SourceDir "rapidjson\license.txt") (Join-Path $NoticesDir "rapidjson-LICENSE.txt")
}
if (Test-Path (Join-Path $SourceDir "uWebSockets\LICENSE")) {
    Copy-Item (Join-Path $SourceDir "uWebSockets\LICENSE") (Join-Path $NoticesDir "uWebSockets-LICENSE.txt")
}
"Qt 6.5.2 is dynamically linked. See Qt licensing documentation." | Set-Content (Join-Path $NoticesDir "Qt-NOTICE.txt")

$ZipPath = Join-Path $ReleaseDir "posagent-win64-dev.zip"
Compress-Archive -Path (Join-Path $PackageDir "*") -DestinationPath $ZipPath -Force
$Hash = (Get-FileHash -Algorithm SHA256 $ZipPath).Hash
"$Hash  posagent-win64-dev.zip" | Set-Content -Encoding ascii (Join-Path $ReleaseDir "SHA256SUMS.txt")

Write-Host "Created: $ZipPath"
Write-Host "Physical USB printing is still required as a real Windows acceptance test."
