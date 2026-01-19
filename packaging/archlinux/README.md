# Arch Linux Package for Sindarin

This directory contains the PKGBUILD template for installing Sindarin on Arch Linux.

## Installation

Download `PKGBUILD` from the GitHub release and build:

```bash
# Download the PKGBUILD
curl -LO https://github.com/RealOrko/sindarin/releases/latest/download/PKGBUILD

# Build and install (will prompt for sudo)
makepkg -si
```

## Dependencies

The package depends on:
- `gcc` - C compiler for compiling generated code
- `zlib` - Compression library for SDK features

## Usage

After installation:

```bash
sn hello.sn -o hello
./hello
```

## Uninstall

```bash
sudo pacman -R sindarin
```

## AUR Submission

To submit to the Arch User Repository (AUR):

1. Create an AUR account at https://aur.archlinux.org
2. Clone the AUR package: `git clone ssh://aur@aur.archlinux.org/sindarin.git`
3. Copy the PKGBUILD and generate `.SRCINFO`: `makepkg --printsrcinfo > .SRCINFO`
4. Commit and push to AUR
