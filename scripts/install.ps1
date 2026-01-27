<#
.SYNOPSIS
    Installs the Sindarin SDK from the latest GitHub release.

.DESCRIPTION
    This script:
    1. Uninstalls any existing Sindarin packages via winget
    2. Downloads the latest winget-manifest.zip from GitHub releases
    3. Extracts and installs using winget with the local manifest
    4. Tests the installation by compiling and running a hello world program

.EXAMPLE
    .\scripts\install.ps1
#>

param(
    [switch]$SkipTest,
    [switch]$SkipUninstall
)

$ErrorActionPreference = "Stop"

$RepoOwner = "SindarinSDK"
$RepoName = "sindarin-compiler"
$ManifestAsset = "winget-manifests.zip"

function Write-Status {
    param([string]$Message, [string]$Type = "Info")

    switch ($Type) {
        "Info"    { Write-Host "[*] $Message" -ForegroundColor Cyan }
        "Success" { Write-Host "[+] $Message" -ForegroundColor Green }
        "Warning" { Write-Host "[!] $Message" -ForegroundColor Yellow }
        "Error"   { Write-Host "[-] $Message" -ForegroundColor Red }
    }
}

function Uninstall-ExistingSindarin {
    Write-Status "Checking for existing Sindarin installations..."

    $packages = winget list --name Sindarin 2>$null | Select-String -Pattern "Sindarin"

    if ($packages) {
        Write-Status "Found existing Sindarin installation(s), uninstalling..." "Warning"

        # Get the package IDs
        $listOutput = winget list --name Sindarin 2>$null
        $lines = $listOutput -split "`n" | Where-Object { $_ -match "Sindarin" }

        foreach ($line in $lines) {
            # Extract the ID (second column)
            if ($line -match "Sindarin\s+(\S+)") {
                $packageId = $Matches[1]
                Write-Status "Uninstalling: $packageId"
                winget uninstall --id $packageId --silent 2>$null
                if ($LASTEXITCODE -eq 0) {
                    Write-Status "Successfully uninstalled $packageId" "Success"
                } else {
                    Write-Status "Failed to uninstall $packageId (may require manual removal)" "Warning"
                }
            }
        }
    } else {
        Write-Status "No existing Sindarin installations found" "Success"
    }
}

function Get-LatestRelease {
    Write-Status "Fetching latest release from GitHub..."

    $apiUrl = "https://api.github.com/repos/$RepoOwner/$RepoName/releases/latest"

    try {
        $headers = @{
            "Accept" = "application/vnd.github.v3+json"
            "User-Agent" = "Sindarin-Installer"
        }

        $release = Invoke-RestMethod -Uri $apiUrl -Headers $headers -Method Get

        Write-Status "Found release: $($release.tag_name)" "Success"

        # Find the winget-manifest.zip asset
        $asset = $release.assets | Where-Object { $_.name -eq $ManifestAsset }

        if (-not $asset) {
            throw "Asset '$ManifestAsset' not found in release $($release.tag_name)"
        }

        return @{
            TagName = $release.tag_name
            AssetUrl = $asset.browser_download_url
            AssetName = $asset.name
        }
    }
    catch {
        throw "Failed to fetch latest release: $_"
    }
}

function Install-FromManifest {
    param([hashtable]$Release)

    $tempDir = Join-Path $env:TEMP "sindarin-install-$(Get-Date -Format 'yyyyMMddHHmmss')"
    $zipPath = Join-Path $tempDir $Release.AssetName
    $extractPath = Join-Path $tempDir "manifest"

    try {
        # Create temp directory
        New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
        Write-Status "Created temp directory: $tempDir"

        # Download the manifest zip
        Write-Status "Downloading $($Release.AssetName) from $($Release.TagName)..."
        Invoke-WebRequest -Uri $Release.AssetUrl -OutFile $zipPath -UseBasicParsing
        Write-Status "Downloaded to: $zipPath" "Success"

        # Extract the zip
        Write-Status "Extracting manifest..."
        Expand-Archive -Path $zipPath -DestinationPath $extractPath -Force
        Write-Status "Extracted to: $extractPath" "Success"

        # Find the manifest directory (should contain .yaml files)
        $manifestDir = Get-ChildItem -Path $extractPath -Directory -Recurse |
            Where-Object { Get-ChildItem -Path $_.FullName -Filter "*.yaml" -File } |
            Select-Object -First 1

        if (-not $manifestDir) {
            # Maybe the yaml files are directly in extractPath
            $yamlFiles = Get-ChildItem -Path $extractPath -Filter "*.yaml" -File -Recurse
            if ($yamlFiles) {
                $manifestDir = $yamlFiles[0].Directory
            } else {
                throw "No manifest files (.yaml) found in the extracted archive"
            }
        }

        Write-Status "Found manifest at: $($manifestDir.FullName)"

        # Install using winget with local manifest
        Write-Status "Installing Sindarin SDK via winget..."
        $installResult = winget install --manifest $manifestDir.FullName --accept-package-agreements --accept-source-agreements 2>&1

        if ($LASTEXITCODE -ne 0) {
            Write-Host $installResult
            throw "winget install failed with exit code $LASTEXITCODE"
        }

        Write-Status "Sindarin SDK installed successfully!" "Success"

        return $tempDir
    }
    catch {
        # Cleanup on failure
        if (Test-Path $tempDir) {
            Remove-Item -Path $tempDir -Recurse -Force -ErrorAction SilentlyContinue
        }
        throw
    }
}

function Test-Installation {
    param([string]$TempDir)

    Write-Status "Testing Sindarin installation..."

    $testDir = if ($TempDir) { $TempDir } else { Join-Path $env:TEMP "sindarin-test-$(Get-Date -Format 'yyyyMMddHHmmss')" }
    $testFile = Join-Path $testDir "hello.sn"
    $testExe = Join-Path $testDir "hello.exe"

    try {
        # Create test directory if needed
        if (-not (Test-Path $testDir)) {
            New-Item -ItemType Directory -Path $testDir -Force | Out-Null
        }

        # Write hello world program
        $helloWorld = @"
fn main(): int {
    println("Hello, World from Sindarin!")
    return 0
}
"@
        Set-Content -Path $testFile -Value $helloWorld -Encoding UTF8
        Write-Status "Created test file: $testFile"

        # Refresh PATH to pick up newly installed sn compiler
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

        # Compile the test program
        Write-Status "Compiling hello.sn..."
        $compileOutput = & sn $testFile -o $testExe 2>&1

        if ($LASTEXITCODE -ne 0) {
            Write-Host $compileOutput
            throw "Compilation failed with exit code $LASTEXITCODE"
        }

        Write-Status "Compilation successful!" "Success"

        # Run the compiled program
        Write-Status "Running hello.exe..."
        $runOutput = & $testExe 2>&1

        if ($LASTEXITCODE -ne 0) {
            Write-Host $runOutput
            throw "Execution failed with exit code $LASTEXITCODE"
        }

        Write-Host ""
        Write-Host "  Output: $runOutput" -ForegroundColor White
        Write-Host ""

        if ($runOutput -match "Hello.*World.*Sindarin") {
            Write-Status "Test passed! Sindarin is working correctly." "Success"
        } else {
            Write-Status "Test completed but output was unexpected" "Warning"
        }

        return $true
    }
    catch {
        Write-Status "Test failed: $_" "Error"
        return $false
    }
    finally {
        # Cleanup test files
        if (Test-Path $testDir) {
            Remove-Item -Path $testDir -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

function Main {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Magenta
    Write-Host "    Sindarin SDK Installer" -ForegroundColor Magenta
    Write-Host "========================================" -ForegroundColor Magenta
    Write-Host ""

    $tempDir = $null

    try {
        # Step 1: Uninstall existing installations
        if (-not $SkipUninstall) {
            Uninstall-ExistingSindarin
        }

        # Step 2: Get latest release info
        $release = Get-LatestRelease

        # Step 3: Download and install from manifest
        $tempDir = Install-FromManifest -Release $release

        # Step 4: Test the installation
        if (-not $SkipTest) {
            $testResult = Test-Installation -TempDir $tempDir
            if (-not $testResult) {
                Write-Status "Installation completed but tests failed" "Warning"
                exit 1
            }
        }

        Write-Host ""
        Write-Status "Sindarin SDK installation complete!" "Success"
        Write-Host ""
        Write-Host "  You can now use 'sn' to compile Sindarin programs." -ForegroundColor White
        Write-Host "  Example: sn myprogram.sn -o myprogram.exe" -ForegroundColor White
        Write-Host ""

    }
    catch {
        Write-Status "Installation failed: $_" "Error"
        exit 1
    }
    finally {
        # Cleanup temp directory
        if ($tempDir -and (Test-Path $tempDir)) {
            Remove-Item -Path $tempDir -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

# Run main
Main
