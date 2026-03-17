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

Write-Host ""
Write-Host "========================================" -ForegroundColor Magenta
Write-Host "  Sindarin Prerequisites Installer" -ForegroundColor Magenta
Write-Host "========================================" -ForegroundColor Magenta
Write-Host ""

# -------------------------------------------------------------------------
# Check for Chocolatey
# -------------------------------------------------------------------------
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
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
# Remove system LLVM (avoids conflicts with LLVM-MinGW)
# -------------------------------------------------------------------------
Write-Status "Checking for conflicting system LLVM..."
if (Test-Path "C:\Program Files\LLVM") {
    Write-Status "Removing system LLVM to avoid conflicts..."
    Remove-Item -Path "C:\Program Files\LLVM" -Recurse -Force
    Write-Status "System LLVM removed" "Success"
} else {
    Write-Status "No system LLVM found, skipping" "Success"
}

# -------------------------------------------------------------------------
# Install LLVM-MinGW
# -------------------------------------------------------------------------
$llvmVersion = "20241217"
$llvmUrl = "https://github.com/mstorsjo/llvm-mingw/releases/download/$llvmVersion/llvm-mingw-$llvmVersion-ucrt-x86_64.zip"
$llvmZip  = "$env:TEMP\llvm-mingw.zip"
$llvmRoot = "C:\llvm-mingw"

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
    Remove-Item -Path $llvmRoot -Recurse -Force
}
Expand-Archive -Path $llvmZip -DestinationPath $llvmRoot -Force
Remove-Item -Path $llvmZip -Force

$llvmDir = Get-ChildItem -Path $llvmRoot -Directory | Select-Object -First 1
$llvmBin = Join-Path $llvmDir.FullName "bin"
Write-Status "LLVM-MinGW installed at: $($llvmDir.FullName)" "Success"

# Add to PATH for the current session
$env:PATH = "$llvmBin;$env:PATH"

# Append to GITHUB_PATH when running in GitHub Actions
if ($env:GITHUB_PATH) {
    Write-Status "Registering LLVM-MinGW in GITHUB_PATH..."
    Add-Content -Path $env:GITHUB_PATH -Value $llvmBin
}

# -------------------------------------------------------------------------
# Install make via Chocolatey
# -------------------------------------------------------------------------
Write-Status "Installing make via Chocolatey..."
choco install make -y
Write-Status "make installed" "Success"

Write-Host ""
Write-Status "Prerequisites installed successfully!" "Success"
Write-Host ""
