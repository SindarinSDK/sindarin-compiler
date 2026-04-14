<#
.SYNOPSIS
    Installs the Sindarin compiler from the latest S3 release.

.DESCRIPTION
    This script:
    1. Downloads the latest Windows release from S3
    2. Extracts it to ~/.sn
    3. Adds ~/.sn/bin to the user PATH (if not already present)

.EXAMPLE
    irm https://raw.githubusercontent.com/SindarinSDK/sindarin-compiler/main/scripts/install.ps1 | iex
#>

$ErrorActionPreference = "Stop"

$S3Bucket = "cryosharp-sindarin-pkg-binaries"
$S3Region = "eu-west-2"
$S3BaseUrl = "https://${S3Bucket}.s3.${S3Region}.amazonaws.com"
$InstallDir = Join-Path $env:USERPROFILE ".sn"
$BinDir = Join-Path $InstallDir "bin"

function Write-Status {
    param([string]$Message, [string]$Type = "Info")

    switch ($Type) {
        "Info"    { Write-Host "[*] $Message" -ForegroundColor Cyan }
        "Success" { Write-Host "[+] $Message" -ForegroundColor Green }
        "Warning" { Write-Host "[!] $Message" -ForegroundColor Yellow }
        "Error"   { Write-Host "[-] $Message" -ForegroundColor Red }
    }
}

function Invoke-Prereqs {
    $prereqsUrl = "https://raw.githubusercontent.com/SindarinSDK/sindarin-compiler/main/scripts/install-prereqs.ps1"
    $localScript = if ($PSScriptRoot) { Join-Path $PSScriptRoot "install-prereqs.ps1" } else { $null }

    if ($localScript -and (Test-Path $localScript)) {
        Write-Status "Running prerequisites installer from $localScript..."
        & $localScript
        if ($LASTEXITCODE) {
            throw "Prerequisites installation failed with exit code $LASTEXITCODE"
        }
    } else {
        Write-Status "Fetching prerequisites installer..."
        $prereqsScript = (Invoke-WebRequest -Uri $prereqsUrl -UseBasicParsing).Content
        Invoke-Expression $prereqsScript
    }
}

function Get-LatestVersion {
    Write-Status "Fetching latest version..."

    $versionUrl = "${S3BaseUrl}/sindarin-compiler/__version__"

    try {
        $version = (Invoke-WebRequest -Uri $versionUrl -UseBasicParsing).Content.Trim()

        if (-not $version) {
            throw "Empty version response"
        }

        Write-Status "Found release: $version" "Success"
        return $version
    }
    catch {
        throw "Failed to fetch latest version: $_"
    }
}

function Install-Sindarin {
    param([string]$TagName)

    # Extract version number from tag (e.g., "0.0.65" from "v0.0.65")
    $versionNum = $TagName -replace '^v', ''

    # Determine the zip name and S3 URL
    $zipName = "sindarin-${versionNum}-windows-x64.zip"
    $downloadUrl = "${S3BaseUrl}/sindarin-compiler/${TagName}/${zipName}"

    $tempDir = Join-Path $env:TEMP "sindarin-install-$(Get-Date -Format 'yyyyMMddHHmmss')"
    $zipPath = Join-Path $tempDir $zipName

    try {
        # Create temp directory
        New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

        # Download the release zip
        Write-Status "Downloading $zipName..."
        Invoke-WebRequest -Uri $downloadUrl -OutFile $zipPath -UseBasicParsing
        Write-Status "Download complete" "Success"

        # Remove existing installation if present
        if (Test-Path $InstallDir) {
            Write-Status "Removing existing installation at $InstallDir..."
            Remove-Item -Path $InstallDir -Recurse -Force
        }

        # Create installation directory
        New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null

        # Extract the zip
        Write-Status "Extracting to $InstallDir..."
        Expand-Archive -Path $zipPath -DestinationPath $InstallDir -Force

        # The zip might contain a nested folder - flatten if needed
        $nestedDirs = Get-ChildItem -Path $InstallDir -Directory
        if ($nestedDirs.Count -eq 1 -and (Test-Path (Join-Path $nestedDirs[0].FullName "bin"))) {
            $nestedPath = $nestedDirs[0].FullName
            Write-Status "Flattening nested directory structure..."

            # Move contents up one level
            Get-ChildItem -Path $nestedPath | Move-Item -Destination $InstallDir -Force
            Remove-Item -Path $nestedPath -Force
        }

        # Verify bin directory exists
        if (-not (Test-Path $BinDir)) {
            throw "Installation failed: bin directory not found at $BinDir"
        }

        # Verify sn.exe exists
        $snExe = Join-Path $BinDir "sn.exe"
        if (-not (Test-Path $snExe)) {
            throw "Installation failed: sn.exe not found at $snExe"
        }

        Write-Status "Extraction complete" "Success"

        return $true
    }
    catch {
        throw "Installation failed: $_"
    }
    finally {
        # Cleanup temp directory
        if (Test-Path $tempDir) {
            Remove-Item -Path $tempDir -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

function Add-ToPath {
    Write-Status "Configuring PATH..."

    # In GitHub Actions, write to GITHUB_PATH so the bin dir is available
    # to all subsequent steps without needing a shell restart.
    if ($env:GITHUB_PATH) {
        Write-Status "GitHub Actions detected, registering in GITHUB_PATH..."
        Add-Content -Path $env:GITHUB_PATH -Value $BinDir
        Write-Status "Added $BinDir to GITHUB_PATH" "Success"
        return
    }

    # Get current user PATH
    $currentPath = [System.Environment]::GetEnvironmentVariable("Path", "User")

    # Split into individual paths and normalize them
    $pathList = if ($currentPath) {
        $currentPath -split ";" | Where-Object { $_ -ne "" } | ForEach-Object { $_.TrimEnd("\") }
    } else {
        @()
    }

    # Normalize our bin directory path for comparison
    $normalizedBinDir = $BinDir.TrimEnd("\")

    # Check if already in PATH (case-insensitive)
    $alreadyInPath = $pathList | Where-Object { $_.ToLower() -eq $normalizedBinDir.ToLower() }

    if ($alreadyInPath) {
        Write-Status "PATH already contains $BinDir" "Success"
        return
    }

    # Add to PATH
    $newPath = if ($currentPath) {
        "$currentPath;$BinDir"
    } else {
        $BinDir
    }

    [System.Environment]::SetEnvironmentVariable("Path", $newPath, "User")
    Write-Status "Added $BinDir to user PATH" "Success"
}

function Main {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Magenta
    Write-Host "    Sindarin Compiler Installer" -ForegroundColor Magenta
    Write-Host "========================================" -ForegroundColor Magenta
    Write-Host ""

    try {
        # Step 1: Install prerequisites (build tools)
        Invoke-Prereqs

        # Step 2: Get latest version from S3
        $tagName = Get-LatestVersion

        # Step 3: Download and install
        Install-Sindarin -TagName $tagName | Out-Null

        # Step 4: Add to PATH (idempotent - won't duplicate)
        Add-ToPath

        Write-Host ""
        Write-Status "Sindarin compiler installed successfully!" "Success"
        Write-Host ""
        Write-Host "  Installed to: $InstallDir" -ForegroundColor White
        Write-Host "  Version:      $tagName" -ForegroundColor White
        Write-Host ""
        Write-Host "  ============================================" -ForegroundColor Yellow
        Write-Host "  IMPORTANT: Please restart your terminal" -ForegroundColor Yellow
        Write-Host "  for PATH changes to take effect." -ForegroundColor Yellow
        Write-Host "  ============================================" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "  After restarting, you can compile Sindarin programs:" -ForegroundColor White
        Write-Host "    sn myprogram.sn -o myprogram.exe" -ForegroundColor White
        Write-Host ""
    }
    catch {
        Write-Status "Installation failed: $_" "Error"
        exit 1
    }
}

# Run main
Main
