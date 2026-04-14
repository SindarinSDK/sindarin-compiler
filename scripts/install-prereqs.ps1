# Sindarin Prerequisites Installer (Windows)
#
# Installs build tools required to compile Sindarin programs on Windows.
# Requires: Chocolatey (https://chocolatey.org)
#
# Usage:
#   scripts\install-prereqs.ps1
#   irm https://raw.githubusercontent.com/SindarinSDK/sindarin-compiler/main/scripts/install-prereqs.ps1 | iex
#

$ErrorActionPreference = "Stop"

function Write-Status {
    param([string]$Message, [string]$Type = "Info")
    switch ($Type) {
        "Info"    { Write-Host "[*] $Message" -ForegroundColor Cyan }
        "Success" { Write-Host "[+] $Message" -ForegroundColor Green }
        "Warning" { Write-Host "[!] $Message" -ForegroundColor Yellow }
        "Error"   { Write-Host "[-] $Message" -ForegroundColor Red }
    }
}

function Test-Command {
    param([string]$Name)
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Test-IsAdministrator {
    $identity  = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Add-PathEntry {
    param([string]$Entry)
    $parts = $env:PATH -split ';'
    if ($parts -notcontains $Entry) {
        $env:PATH = "$Entry;$env:PATH"
    }
}

$installed = @()
$skipped   = @()

Write-Host ""
Write-Host "========================================" -ForegroundColor Magenta
Write-Host "  Sindarin Prerequisites Installer" -ForegroundColor Magenta
Write-Host "========================================" -ForegroundColor Magenta
Write-Host ""

# -------------------------------------------------------------------------
# Preflight: administrator check
# -------------------------------------------------------------------------
# GitHub Actions runners always run elevated; skip the check there so the
# script works both on developer machines and in CI.
if (-not $env:GITHUB_PATH -and -not (Test-IsAdministrator)) {
    Write-Status "This script must be run from an elevated (Administrator) PowerShell." "Error"
    Write-Host ""
    Write-Host "  Installing LLVM-MinGW under C:\ and running 'choco install' both" -ForegroundColor White
    Write-Host "  require administrator rights. Re-open PowerShell as Administrator" -ForegroundColor White
    Write-Host "  and re-run this script." -ForegroundColor White
    Write-Host ""
    exit 1
}

# -------------------------------------------------------------------------
# Check for Chocolatey
# -------------------------------------------------------------------------
if (-not (Test-Command choco)) {
    Write-Status "Chocolatey is not installed." "Error"
    Write-Host ""
    Write-Host "  Chocolatey is required to install Sindarin prerequisites on Windows." -ForegroundColor White
    Write-Host "  To install Chocolatey, open an elevated (Administrator) PowerShell and run:" -ForegroundColor White
    Write-Host ""
    Write-Host "    Set-ExecutionPolicy Bypass -Scope Process -Force" -ForegroundColor Yellow
    Write-Host "    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072" -ForegroundColor Yellow
    Write-Host "    iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  Or visit: https://chocolatey.org/install" -ForegroundColor White
    Write-Host ""
    Write-Host "  Then re-run this script." -ForegroundColor White
    Write-Host ""
    exit 1
}

# -------------------------------------------------------------------------
# Remove system LLVM only when it would shadow LLVM-MinGW on PATH
# -------------------------------------------------------------------------
Write-Status "Checking for conflicting system LLVM..."
$systemLlvm    = "C:\Program Files\LLVM"
$systemLlvmBin = Join-Path $systemLlvm "bin"
if (Test-Path $systemLlvm) {
    $onPath = ($env:PATH -split ';') -contains $systemLlvmBin
    if ($onPath) {
        Write-Status "Removing system LLVM at $systemLlvm to avoid conflicts..."
        $sysLlvmProcs = Get-Process -ErrorAction SilentlyContinue |
            Where-Object {
                try { $_.Path -and $_.Path.StartsWith($systemLlvm, [StringComparison]::OrdinalIgnoreCase) }
                catch { $false }
            }
        if ($sysLlvmProcs) {
            Write-Status "Stopping processes running from $systemLlvm`: $($sysLlvmProcs.Name -join ', ')..." "Warning"
            $sysLlvmProcs | Stop-Process -Force -ErrorAction SilentlyContinue
            Start-Sleep -Seconds 2
        }
        Remove-Item -Path $systemLlvm -Recurse -Force
        $installed += "system LLVM removed"
    } else {
        Write-Status "System LLVM present but not on PATH, leaving it alone" "Success"
        $skipped += "system LLVM removal (not on PATH)"
    }
} else {
    Write-Status "No system LLVM found, skipping" "Success"
    $skipped += "system LLVM removal (not installed)"
}

# -------------------------------------------------------------------------
# Install LLVM-MinGW
# -------------------------------------------------------------------------
$llvmVersion    = "20241217"
$llvmUrl        = "https://github.com/mstorsjo/llvm-mingw/releases/download/$llvmVersion/llvm-mingw-$llvmVersion-ucrt-x86_64.zip"
$llvmZip        = "$env:TEMP\llvm-mingw.zip"
$llvmRoot       = "C:\llvm-mingw"
$llvmVersionTag = Join-Path $llvmRoot ".sindarin-version"

$needLlvmInstall = $true
if ((Test-Path $llvmRoot) -and (Test-Path $llvmVersionTag)) {
    $existing = (Get-Content -Path $llvmVersionTag -Raw).Trim()
    if ($existing -eq $llvmVersion) {
        $needLlvmInstall = $false
    } else {
        Write-Status "LLVM-MinGW at $llvmRoot is version '$existing', need '$llvmVersion'" "Warning"
    }
}

if ($needLlvmInstall) {
    Write-Status "Downloading LLVM-MinGW $llvmVersion..."
    try {
        Invoke-WebRequest -Uri $llvmUrl -OutFile $llvmZip
        Write-Status "Download complete" "Success"
    } catch {
        Write-Status "Failed to download LLVM-MinGW: $_" "Error"
        exit 1
    }

    Write-Status "Extracting LLVM-MinGW to $llvmRoot..."
    if (Test-Path $llvmRoot) {
        # Kill any processes running from the LLVM-MinGW directory (e.g. clangd
        # launched by an editor) so we can delete the folder.
        $llvmProcs = Get-Process -ErrorAction SilentlyContinue |
            Where-Object {
                try { $_.Path -and $_.Path.StartsWith($llvmRoot, [StringComparison]::OrdinalIgnoreCase) }
                catch { $false }
            }
        if ($llvmProcs) {
            Write-Status "Stopping processes running from $llvmRoot`: $($llvmProcs.Name -join ', ')..." "Warning"
            $llvmProcs | Stop-Process -Force -ErrorAction SilentlyContinue
            Start-Sleep -Seconds 2
        }
        Remove-Item -Path $llvmRoot -Recurse -Force
    }
    Expand-Archive -Path $llvmZip -DestinationPath $llvmRoot -Force
    Remove-Item -Path $llvmZip -Force

    Set-Content -Path $llvmVersionTag -Value $llvmVersion -Encoding ASCII
    $installed += "LLVM-MinGW $llvmVersion"
} else {
    Write-Status "LLVM-MinGW $llvmVersion already installed at $llvmRoot" "Success"
    $skipped += "LLVM-MinGW $llvmVersion (already installed)"
}

$llvmDir = Get-ChildItem -Path $llvmRoot -Directory | Select-Object -First 1
if (-not $llvmDir) {
    Write-Status "LLVM-MinGW directory layout unexpected under $llvmRoot" "Error"
    exit 1
}
$llvmBin = Join-Path $llvmDir.FullName "bin"
Write-Status "LLVM-MinGW bin: $llvmBin" "Success"

# Add to PATH for the current session
Add-PathEntry $llvmBin

# Append to GITHUB_PATH when running in GitHub Actions
if ($env:GITHUB_PATH) {
    Write-Status "Registering LLVM-MinGW in GITHUB_PATH..."
    Add-Content -Path $env:GITHUB_PATH -Value $llvmBin
}

# -------------------------------------------------------------------------
# Install make / ninja / cmake via Chocolatey (only what's missing)
# -------------------------------------------------------------------------
$chocoTools = @("make", "ninja", "cmake")
$missingTools = @($chocoTools | Where-Object { -not (Test-Command $_) })

foreach ($tool in $chocoTools) {
    if ($missingTools -contains $tool) { continue }
    Write-Status "$tool already present, skipping" "Success"
    $skipped += "$tool (already installed)"
}

if ($missingTools.Count -gt 0) {
    Write-Status "Installing via Chocolatey: $($missingTools -join ', ')..."
    choco install @missingTools -y --no-progress
    if ($LASTEXITCODE -ne 0) {
        Write-Status "choco install failed (exit $LASTEXITCODE)" "Error"
        exit 1
    }
    Write-Status "Build tools installed" "Success"
    $installed += $missingTools
} else {
    Write-Status "All Chocolatey build tools already present" "Success"
}

# -------------------------------------------------------------------------
# Install vcpkg
# -------------------------------------------------------------------------
$vcpkgRoot = "$env:USERPROFILE\vcpkg"

if (Test-Path "$vcpkgRoot\vcpkg.exe") {
    Write-Status "vcpkg already installed at $vcpkgRoot" "Success"
    $skipped += "vcpkg (already installed)"
} else {
    Write-Status "Installing vcpkg to $vcpkgRoot..."
    git clone https://github.com/microsoft/vcpkg.git $vcpkgRoot
    & "$vcpkgRoot\bootstrap-vcpkg.bat" -disableMetrics
    Write-Status "vcpkg installed" "Success"
    $installed += "vcpkg"
}

# Export for current session
$env:VCPKG_ROOT = $vcpkgRoot
Add-PathEntry $vcpkgRoot

# Register in GITHUB_ENV/GITHUB_PATH when running in GitHub Actions
if ($env:GITHUB_ENV) {
    $vcpkgRootFwd = $vcpkgRoot.Replace('\', '/')
    "VCPKG_ROOT=$vcpkgRootFwd" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
}
if ($env:GITHUB_PATH) {
    Add-Content -Path $env:GITHUB_PATH -Value $vcpkgRoot
}

# -------------------------------------------------------------------------
# Summary
# -------------------------------------------------------------------------
Write-Host ""
Write-Status "Prerequisites installed successfully!" "Success"
Write-Host ""
if ($installed.Count -gt 0) {
    Write-Host "  Installed:" -ForegroundColor Green
    foreach ($item in $installed) { Write-Host "    - $item" -ForegroundColor Green }
}
if ($skipped.Count -gt 0) {
    Write-Host "  Skipped (already present):" -ForegroundColor DarkGray
    foreach ($item in $skipped) { Write-Host "    - $item" -ForegroundColor DarkGray }
}
Write-Host ""
exit 0
