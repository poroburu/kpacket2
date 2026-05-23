# Build kpacket2 (Ashita C++ plugin) and deploy to HorizonXI plugins folder.
#
# Usage:
#   .\build.ps1                    # Release build (default)
#   .\build.ps1 -Configuration Debug
#   .\build.ps1 -SkipConfigure     # rebuild only
#   .\build.ps1 -BuildExamples     # also build packet_monitor tools

[CmdletBinding()]
param(
    [ValidateSet("Release", "Debug")]
    [string] $Configuration = "Release",

    [switch] $SkipConfigure,
    [switch] $BuildExamples,
    [switch] $SkipVcVars
)

$ErrorActionPreference = "Stop"

$RepoRoot = $PSScriptRoot
$PresetName = if ($Configuration -eq "Debug") { "x86-debug-win" } else { "x86-release-win" }
$BuildDir = Join-Path $RepoRoot "out\build\$PresetName"
$PluginPath = Join-Path $env:APPDATA "HorizonXI-Launcher\HorizonXI\Game\plugins"
$SdkPath = Join-Path $PluginPath "sdk"
$VcpkgRoot = Join-Path $env:USERPROFILE "git\vcpkg"
$BinOutput = Join-Path $RepoRoot "bin\kpacket.dll"
$DeployedDll = Join-Path $PluginPath "kpacket.dll"

function Write-Step([string] $Message) {
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Test-Command([string] $Name) {
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Ensure-VcpkgPackages {
    if (-not (Test-Path $VcpkgRoot)) {
        throw "vcpkg not found at $VcpkgRoot. Clone it or update CMakePresets.json VCPKG_ROOT."
    }

    $vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
    if (-not (Test-Path $vcpkgExe)) {
        throw "vcpkg.exe not found at $vcpkgExe"
    }

    Write-Step "Ensuring vcpkg packages (x86-windows): cppzmq, nlohmann-json"
    & $vcpkgExe install cppzmq:x86-windows nlohmann-json:x86-windows
    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg install failed with exit code $LASTEXITCODE"
    }
}

function Enter-MsvcX86Environment {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        throw "cl.exe not on PATH and vswhere not found. Open an x86 Native Tools prompt or install Visual Studio C++ workload."
    }

    $vsPath = & $vswhere -latest -property installationPath
    if (-not $vsPath) {
        throw "Visual Studio installation not found."
    }

    $vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
    if (-not (Test-Path $vcvars)) {
        throw "vcvarsall.bat not found at $vcvars"
    }

    Write-Step "Loading MSVC x86 environment via vcvarsall.bat"
    $extra = @()
    if ($SkipConfigure) { $extra += "-SkipConfigure" }
    if ($BuildExamples) { $extra += "-BuildExamples" }
    $extra += "-SkipVcVars"
    $argList = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $PSCommandPath, "-Configuration", $Configuration) + $extra
    $argString = ($argList | ForEach-Object { if ($_ -match '\s') { "`"$_`"" } else { $_ } }) -join ' '
    cmd /c "@call `"$vcvars`" x86 >nul && powershell $argString"
    exit $LASTEXITCODE
}

if (-not $SkipVcVars -and -not (Test-Command cl.exe)) {
    Enter-MsvcX86Environment
}

Write-Step "kpacket2 build ($Configuration)"
Write-Host "Repo:       $RepoRoot"
Write-Host "Preset:     $PresetName"
Write-Host "Build dir:  $BuildDir"
Write-Host "Deploy to:  $PluginPath"

if (-not (Test-Command cmake)) {
    throw "cmake not found on PATH."
}

if (-not (Test-Path (Join-Path $SdkPath "Ashita.h"))) {
    throw "Ashita SDK not found at $SdkPath (expected Ashita.h). Install HorizonXI / Ashita SDK first."
}

if (-not (Test-Path $PluginPath)) {
    New-Item -ItemType Directory -Path $PluginPath -Force | Out-Null
    Write-Host "Created plugin directory: $PluginPath"
}

Ensure-VcpkgPackages

$env:ASHITA4_PLUGIN_PATH = $PluginPath
$env:ASHITA4_SDK_PATH = $SdkPath
$env:VCPKG_ROOT = $VcpkgRoot

function Deploy-Plugin {
    if (-not (Test-Path $BinOutput)) {
        return $false
    }

    Write-Step "Deploying plugin to HorizonXI"
    try {
        Copy-Item -Path $BinOutput -Destination $DeployedDll -Force -ErrorAction Stop

        $zmqDll = Join-Path $VcpkgRoot "installed\x86-windows\bin\libzmq-mt-4_3_5.dll"
        if ($Configuration -eq "Debug") {
            $zmqDll = Join-Path $VcpkgRoot "installed\x86-windows\debug\bin\libzmq-mt-gd-4_3_5.dll"
        }
        if (Test-Path $zmqDll) {
            Copy-Item -Path $zmqDll -Destination $PluginPath -Force -ErrorAction Stop
        }

        return $true
    }
    catch {
        Write-Warning "Could not overwrite $DeployedDll`: $($_.Exception.Message)"
        Write-Warning "Close HorizonXI/Ashita, then deploy with:"
        Write-Host "  Copy-Item '$BinOutput' '$DeployedDll' -Force" -ForegroundColor Yellow
        return $false
    }
}

Push-Location $RepoRoot
try {
    if (-not $SkipConfigure) {
        Write-Step "Configuring CMake preset: $PresetName"
        cmake --preset $PresetName --fresh
        if ($LASTEXITCODE -ne 0) {
            throw "cmake configure failed with exit code $LASTEXITCODE"
        }
    }

    Write-Step "Building ($Configuration)"
    cmake --build $BuildDir --config $Configuration
    $buildExit = $LASTEXITCODE

    if ($BuildExamples) {
        Write-Step "Building examples (packet_monitor)"
        cmake --build $BuildDir --config $Configuration --target packet_monitor packet_monitor_ftxui
        if ($LASTEXITCODE -ne 0) {
            throw "example build failed with exit code $LASTEXITCODE"
        }
    }
}
finally {
    Pop-Location
}

if (-not (Test-Path $BinOutput)) {
    if ($buildExit -ne 0) {
        throw "cmake build failed with exit code $buildExit"
    }
    throw "Build finished but $BinOutput was not produced."
}

if ($buildExit -ne 0) {
    Write-Warning "CMake reported exit code $buildExit (often the POST_BUILD copy step when the game has the plugin loaded)."
}

$deployed = Deploy-Plugin

Write-Step "Verifying outputs"

$found = @()
if (Test-Path $BinOutput) { $found += $BinOutput }
if (Test-Path $DeployedDll) { $found += $DeployedDll }

foreach ($path in $found) {
    $item = Get-Item $path
    Write-Host "OK  $($item.FullName)  ($([math]::Round($item.Length / 1KB, 1)) KB, $($item.LastWriteTime))"
}

if (-not $deployed) {
    Write-Host ""
    Write-Host "Build artifact ready in bin\, but deploy was skipped because the game has kpacket.dll loaded." -ForegroundColor Yellow
    exit 2
}

Write-Host ""
Write-Host "Build succeeded. In game: /load kpacket" -ForegroundColor Green
Write-Host "Test:  dotnet run --project C:\Users\porob\git\kparser2\kparser2.Cli\kparser2.Cli.fsproj -- hello"
