# Homebrew Formula for Sindarin

This directory contains the formula template for installing Sindarin via Homebrew on macOS.

## Installation

Download `sindarin.rb` from the GitHub release and install:

```bash
# Download the formula
curl -LO https://github.com/RealOrko/sindarin/releases/download/0.0.25-alpha/sindarin.rb

# Install from local formula
brew install --formula ./sindarin.rb
```

## Dependencies

The formula depends on Xcode Command Line Tools for the clang compiler. If not installed:

```bash
xcode-select --install
```

## Usage

After installation:

```bash
sn hello.sn -o hello
./hello
```

## Uninstall

```bash
brew uninstall sindarin
```
