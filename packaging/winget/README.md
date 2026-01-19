# Winget Package for Sindarin

This directory contains manifest templates for publishing Sindarin to the Windows Package Manager (winget).

## Package Identifier

The package identifier is `SindarinSDK.Sindarin` (Publisher.Package format is required by winget).
Users install with: `winget install SindarinSDK.Sindarin`

## Automated Release

The release workflow automatically generates ready-to-submit winget manifests with the correct version and SHA256 hash. These are uploaded as the `winget-manifests` artifact.

## Install from Local Manifest

Users can download `winget-manifests.zip` from the GitHub release and install directly:

```powershell
# Download winget-manifests.zip from the release page
Invoke-WebRequest -Uri "https://github.com/RealOrko/sindarin/releases/download/0.0.9/winget-manifests.zip" -OutFile winget-manifests.zip
Expand-Archive winget-manifests.zip -DestinationPath winget-manifests
winget install --manifest ./winget-manifests
```

This installs Sindarin without waiting for the package to be published to the winget repository.

## Submitting to winget-pkgs

### 1. Download manifests

Download the `winget-manifests` artifact from the GitHub release workflow.

### 2. Fork and clone winget-pkgs

```bash
git clone https://github.com/YOUR_USERNAME/winget-pkgs.git
cd winget-pkgs
```

### 3. Create manifest directory

```bash
VERSION="0.0.9"  # Replace with actual version
mkdir -p manifests/s/SindarinSDK/Sindarin/$VERSION
```

### 4. Copy manifests

Copy the three YAML files from the artifact to the new directory.

### 5. Validate manifests

```powershell
winget validate --manifest manifests/s/SindarinSDK/Sindarin/$VERSION
```

### 6. Test installation

```powershell
winget install --manifest manifests/s/SindarinSDK/Sindarin/$VERSION
```

### 7. Submit PR

Commit, push, and create a pull request to microsoft/winget-pkgs.

## Dependencies

The package declares this dependency which winget will install automatically:
- `MartinStorsjo.LLVM-MinGW.UCRT` - LLVM/Clang toolchain (provides clang for compiling generated C code)

If needed, you can install it manually:

```powershell
winget install MartinStorsjo.LLVM-MinGW.UCRT
```

## Using wingetcreate

You can also use [wingetcreate](https://github.com/microsoft/winget-create) to automate submission:

```powershell
# Update existing
wingetcreate update SindarinSDK.Sindarin --version 0.0.10 --urls https://github.com/RealOrko/sindarin/releases/download/0.0.10/sindarin-0.0.10-windows-x64.zip
```
