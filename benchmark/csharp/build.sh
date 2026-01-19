#!/bin/bash
# Build script for C# benchmarks

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building C# benchmarks..."

# Compile each file to an executable using dotnet script/CSC
# Using mcs (Mono C# compiler) if available, otherwise csc
if command -v csc &> /dev/null; then
    csc -optimize+ -out:Fib.exe Fib.cs
    csc -optimize+ -out:Primes.exe Primes.cs
    csc -optimize+ -out:Strings.exe Strings.cs
    csc -optimize+ -out:Arrays.exe Arrays.cs
elif command -v mcs &> /dev/null; then
    mcs -optimize+ -out:Fib.exe Fib.cs
    mcs -optimize+ -out:Primes.exe Primes.cs
    mcs -optimize+ -out:Strings.exe Strings.cs
    mcs -optimize+ -out:Arrays.exe Arrays.cs
else
    # Use dotnet to build - create temporary projects
    for prog in Fib Primes Strings Arrays; do
        mkdir -p "$prog"
        cp "$prog.cs" "$prog/Program.cs"
        cat > "$prog/$prog.csproj" << EOF
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>Exe</OutputType>
    <TargetFramework>net9.0</TargetFramework>
    <ImplicitUsings>disable</ImplicitUsings>
    <Nullable>disable</Nullable>
  </PropertyGroup>
</Project>
EOF
        dotnet build "$prog/$prog.csproj" -c Release -o "$prog/bin" --nologo -v q
    done
fi

echo "Build complete."
