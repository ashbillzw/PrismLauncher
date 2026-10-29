#requires -Version 5.1
[CmdletBinding()]
param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$RuntimeDir,
    [Parameter(Mandatory = $true)][string]$NsisCompiler,
    [string]$VCRuntimeDir
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Join-Path $PSScriptRoot '..\..'
}
$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $RepoRoot 'build-ashbill' }
if ([string]::IsNullOrWhiteSpace($RuntimeDir)) { $RuntimeDir = Join-Path $RepoRoot 'install-niijima' }
$BuildDir = (Resolve-Path -LiteralPath $BuildDir).Path
$RuntimeDir = (Resolve-Path -LiteralPath $RuntimeDir).Path
$NsisCompiler = (Resolve-Path -LiteralPath $NsisCompiler).Path
$installerScript = Join-Path $BuildDir 'program_info\win_install.nsi'
if (-not (Test-Path -LiteralPath $installerScript -PathType Leaf)) {
    throw 'Configure the niijima CMake preset first: generated program_info/win_install.nsi is missing.'
}
$scriptText = Get-Content -LiteralPath $installerScript -Raw
if (-not $scriptText.Contains('Software\NiijimaLauncher')) {
    throw 'Refusing to package an installer without the NiijimaLauncher registry namespace.'
}
$binaryName = 'niijima-launcher'
foreach ($file in @("$binaryName.exe", "${binaryName}_filelink.exe", 'Qt6Core.dll', 'Qt6Widgets.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $RuntimeDir $file) -PathType Leaf)) {
        throw "Required runtime file is missing: $file. Build and cmake --install the niijima preset first."
    }
}
if (Test-Path -LiteralPath (Join-Path $RuntimeDir 'prismlauncher.exe')) {
    throw 'Refusing a runtime mixed with official Prism binaries. Use the separate install-niijima directory.'
}

if ([string]::IsNullOrWhiteSpace($VCRuntimeDir)) {
    if ([string]::IsNullOrWhiteSpace($env:VCToolsRedistDir)) {
        throw 'Pass -VCRuntimeDir pointing to Microsoft.VC143.CRT, or run from a VS Developer PowerShell.'
    }
    $VCRuntimeDir = Join-Path $env:VCToolsRedistDir 'x64\Microsoft.VC143.CRT'
}
$VCRuntimeDir = (Resolve-Path -LiteralPath $VCRuntimeDir).Path
foreach ($file in @('vcruntime140.dll', 'msvcp140.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $VCRuntimeDir $file) -PathType Leaf)) {
        throw "Required app-local MSVC runtime is missing: $file"
    }
}

# A new staging folder on each run prevents stale payloads without recursively deleting anything.
$distDir = Join-Path $BuildDir 'dist'
New-Item -ItemType Directory -Path $distDir -Force | Out-Null
$payloadDir = Join-Path $distDir ('payload-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $payloadDir | Out-Null
$allowedDirectories = @('iconengines', 'imageformats', 'jars', 'platforms', 'styles', 'tls', 'translations', 'licenses')
$allowedFiles = @("$binaryName.exe", "${binaryName}_filelink.exe", 'qt.conf', 'qtlogging.ini', 'README.md', 'LICENSE', 'COPYING.md')

foreach ($entry in Get-ChildItem -LiteralPath $RuntimeDir -Force) {
    if (($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Refusing a linked runtime item: $($entry.Name)"
    }
    if ($entry.PSIsContainer) {
        if ($entry.Name -notin $allowedDirectories) {
            throw "Unexpected runtime directory; refusing to package possible user data: $($entry.Name)"
        }
        $linkedChild = Get-ChildItem -LiteralPath $entry.FullName -Recurse -Force | Where-Object {
            ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
        } | Select-Object -First 1
        if ($linkedChild) { throw "Refusing a linked runtime item: $($linkedChild.FullName)" }
        Copy-Item -LiteralPath $entry.FullName -Destination (Join-Path $payloadDir $entry.Name) -Recurse
    } elseif ($entry.Name -in @('niijimalauncher.exe', 'niijimalauncher_filelink.exe')) {
        # cmake --install may leave binaries from the old filename; never ship both versions.
        Write-Verbose "Skipping superseded preview executable: $($entry.Name)"
        continue
    } elseif ($entry.Extension -eq '.pdb' -or $entry.Name -eq 'manifest.txt') {
        # Debug symbols and CI lists are not runtime payloads.
        continue
    } elseif ($entry.Name -in $allowedFiles -or $entry.Extension -eq '.dll') {
        Copy-Item -LiteralPath $entry.FullName -Destination (Join-Path $payloadDir $entry.Name)
    } else {
        throw "Unexpected runtime file; refusing to package settings or updater binaries: $($entry.Name)"
    }
}
foreach ($dll in Get-ChildItem -LiteralPath $VCRuntimeDir -Filter '*.dll' -File) {
    Copy-Item -LiteralPath $dll.FullName -Destination (Join-Path $payloadDir $dll.Name) -Force
}
$qtConf = Join-Path $payloadDir 'qt.conf'
if (-not (Test-Path -LiteralPath $qtConf)) {
    [IO.File]::WriteAllText($qtConf, "[Paths]`nPrefix=.`nPlugins=.`n", [Text.UTF8Encoding]::new($false))
}

# Always ship the fork source link and unmodified upstream license/notice files.
Copy-Item -LiteralPath (Join-Path $RepoRoot 'program_info\niijima\README-runtime.md') -Destination (Join-Path $payloadDir 'README.md') -Force
foreach ($notice in @('LICENSE', 'COPYING.md')) {
    Copy-Item -LiteralPath (Join-Path $RepoRoot $notice) -Destination (Join-Path $payloadDir $notice) -Force
}

$licenseDir = Join-Path $payloadDir 'licenses'
New-Item -ItemType Directory -Path $licenseDir -Force | Out-Null
foreach ($name in @('LICENSE', 'COPYING.md')) {
    $source = Join-Path $RepoRoot $name
    if (Test-Path -LiteralPath $source -PathType Leaf) {
        Copy-Item -LiteralPath $source -Destination (Join-Path $licenseDir ('PrismLauncher-' + $name)) -Force
    }
}
$fontDir = Join-Path $RepoRoot 'launcher\resources\niijima'
Copy-Item -LiteralPath (Join-Path $fontDir 'OFL.txt') -Destination (Join-Path $licenseDir 'ZenMaruGothic-OFL.txt') -Force
Copy-Item -LiteralPath (Join-Path $fontDir 'NotoSansSC-OFL.txt') -Destination (Join-Path $licenseDir 'NotoSansSC-OFL.txt') -Force

function ConvertTo-NsisString([string]$Value) {
    if ($Value.Contains("`r") -or $Value.Contains("`n")) { throw 'Newlines are not allowed in payload paths.' }
    return $Value.Replace('$', '$$').Replace('"', '$\"')
}
$prefix = $payloadDir.TrimEnd('\') + '\'
$uninstallLines = New-Object 'System.Collections.Generic.List[string]'
$manifestLines = New-Object 'System.Collections.Generic.List[string]'
$uninstallLines.Add('; Generated from this installer payload; never add recursive directory deletion.')
foreach ($file in Get-ChildItem -LiteralPath $payloadDir -File -Recurse | Sort-Object FullName) {
    $relative = $file.FullName.Substring($prefix.Length)
    $escaped = ConvertTo-NsisString $relative
    $uninstallLines.Add(('Delete "$INSTDIR\{0}"' -f $escaped))
    $manifestLines.Add($relative)
}
foreach ($directory in Get-ChildItem -LiteralPath $payloadDir -Directory -Recurse | Sort-Object { $_.FullName.Length } -Descending) {
    $relative = $directory.FullName.Substring($prefix.Length)
    $escaped = ConvertTo-NsisString $relative
    $uninstallLines.Add(('RMDir "$INSTDIR\{0}"' -f $escaped))
}
$uninstallFile = Join-Path $payloadDir '..\uninstall_files.nsh'
$uninstallFile = [IO.Path]::GetFullPath($uninstallFile)
[IO.File]::WriteAllLines($uninstallFile, $uninstallLines, [Text.UTF8Encoding]::new($true))
[IO.File]::WriteAllLines((Join-Path $distDir 'payload-manifest.txt'), $manifestLines, [Text.UTF8Encoding]::new($true))

& $NsisCompiler '/V3' '/INPUTCHARSET' 'UTF8' '-NOCD' "/DRUNTIME_DIR=$payloadDir" "/DUNINSTALL_FILES=$uninstallFile" $installerScript
if ($LASTEXITCODE -ne 0) { throw "NSIS failed with exit code $LASTEXITCODE" }
$installer = Join-Path $distDir 'NiijimaLauncher-Setup.exe'
if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) { throw 'NSIS returned success but expected installer is missing.' }
Get-Item -LiteralPath $installer | Select-Object FullName,Length,LastWriteTime
Write-Output "Payload manifest: $(Join-Path $distDir 'payload-manifest.txt')"
Write-Output 'Build only: the installer has not been executed or installed.'
