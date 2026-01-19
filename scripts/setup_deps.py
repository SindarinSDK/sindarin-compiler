#!/usr/bin/env python3
"""
Cross-platform dependency setup for Sindarin compiler.

This script handles the installation of build dependencies across all supported
platforms (Linux, macOS, Windows) using a unified Python interface.

Usage:
    python scripts/setup_deps.py [options]

Options:
    --vcpkg           Use vcpkg for all platforms (default for Windows)
    --system          Use system package manager (default for Linux/macOS)
    --vcpkg-root PATH Path to vcpkg installation
    --check           Check if dependencies are installed (don't install)
    --verbose         Show detailed output
"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional, List, Tuple


def is_windows() -> bool:
    return platform.system() == 'Windows'


def is_macos() -> bool:
    return platform.system() == 'Darwin'


def is_linux() -> bool:
    return platform.system() == 'Linux'


def run_command(cmd: List[str], check: bool = True, capture: bool = False,
                env: Optional[dict] = None) -> Tuple[int, str, str]:
    """Run a command and return (exit_code, stdout, stderr)."""
    try:
        result = subprocess.run(
            cmd,
            check=check,
            capture_output=capture,
            text=True,
            env=env or os.environ
        )
        return 0, result.stdout or '', result.stderr or ''
    except subprocess.CalledProcessError as e:
        return e.returncode, e.stdout or '', e.stderr or ''
    except FileNotFoundError:
        return -1, '', f'Command not found: {cmd[0]}'


def find_executable(name: str) -> Optional[str]:
    """Find an executable in PATH."""
    return shutil.which(name)


class DependencyChecker:
    """Check for required dependencies."""

    REQUIRED_TOOLS = {
        'cmake': ('cmake', '--version'),
        'ninja': ('ninja', '--version'),
    }

    REQUIRED_COMPILERS = {
        'gcc': ('gcc', '--version'),
        'clang': ('clang', '--version'),
    }

    def check_tool(self, name: str) -> bool:
        """Check if a tool is available."""
        if name not in self.REQUIRED_TOOLS:
            return find_executable(name) is not None

        cmd_name, version_arg = self.REQUIRED_TOOLS[name]
        path = find_executable(cmd_name)
        if not path:
            return False

        exit_code, _, _ = run_command([path, version_arg], check=False, capture=True)
        return exit_code == 0

    def check_compiler(self) -> Tuple[bool, str]:
        """Check if at least one compiler is available."""
        for name, (cmd, arg) in self.REQUIRED_COMPILERS.items():
            path = find_executable(cmd)
            if path:
                exit_code, _, _ = run_command([path, arg], check=False, capture=True)
                if exit_code == 0:
                    return True, name
        return False, ''

    def check_all(self, verbose: bool = False) -> bool:
        """Check all dependencies."""
        all_ok = True

        print("Checking build tools...")
        for tool in ['cmake', 'ninja']:
            ok = self.check_tool(tool)
            status = 'OK' if ok else 'MISSING'
            print(f"  {tool}: {status}")
            if not ok:
                all_ok = False

        print("\nChecking compilers...")
        compiler_ok, compiler_name = self.check_compiler()
        if compiler_ok:
            print(f"  Found: {compiler_name}")
        else:
            print("  No C compiler found (need gcc or clang)")
            all_ok = False

        return all_ok


class LinuxInstaller:
    """Install dependencies on Linux using apt or dnf."""

    def detect_package_manager(self) -> Optional[str]:
        """Detect the system package manager."""
        if find_executable('apt-get'):
            return 'apt'
        elif find_executable('dnf'):
            return 'dnf'
        elif find_executable('yum'):
            return 'yum'
        elif find_executable('pacman'):
            return 'pacman'
        return None

    def install(self, verbose: bool = False) -> bool:
        """Install dependencies using the system package manager."""
        pm = self.detect_package_manager()
        if not pm:
            print("Error: No supported package manager found")
            return False

        print(f"Using package manager: {pm}")

        if pm == 'apt':
            packages = ['build-essential', 'cmake', 'ninja-build', 'zlib1g-dev']
            cmd = ['sudo', 'apt-get', 'update']
            print("Updating package lists...")
            run_command(cmd, check=False)

            cmd = ['sudo', 'apt-get', 'install', '-y'] + packages
            print(f"Installing: {', '.join(packages)}")
            exit_code, _, _ = run_command(cmd, check=False)
            return exit_code == 0

        elif pm == 'dnf':
            packages = ['gcc', 'gcc-c++', 'cmake', 'ninja-build', 'zlib-devel']
            cmd = ['sudo', 'dnf', 'install', '-y'] + packages
            print(f"Installing: {', '.join(packages)}")
            exit_code, _, _ = run_command(cmd, check=False)
            return exit_code == 0

        elif pm == 'yum':
            packages = ['gcc', 'gcc-c++', 'cmake', 'ninja-build', 'zlib-devel']
            cmd = ['sudo', 'yum', 'install', '-y'] + packages
            print(f"Installing: {', '.join(packages)}")
            exit_code, _, _ = run_command(cmd, check=False)
            return exit_code == 0

        elif pm == 'pacman':
            packages = ['base-devel', 'cmake', 'ninja', 'zlib']
            cmd = ['sudo', 'pacman', '-S', '--noconfirm'] + packages
            print(f"Installing: {', '.join(packages)}")
            exit_code, _, _ = run_command(cmd, check=False)
            return exit_code == 0

        return False


class MacOSInstaller:
    """Install dependencies on macOS using Homebrew."""

    def check_homebrew(self) -> bool:
        """Check if Homebrew is installed."""
        return find_executable('brew') is not None

    def install(self, verbose: bool = False) -> bool:
        """Install dependencies using Homebrew."""
        if not self.check_homebrew():
            print("Error: Homebrew not found")
            print("Install it from: https://brew.sh")
            return False

        packages = ['cmake', 'ninja', 'coreutils', 'zlib']
        cmd = ['brew', 'install'] + packages
        print(f"Installing: {', '.join(packages)}")
        exit_code, _, _ = run_command(cmd, check=False)
        return exit_code == 0


class WindowsInstaller:
    """Install dependencies on Windows using winget and vcpkg."""

    def __init__(self, vcpkg_root: Optional[str] = None):
        self.vcpkg_root = vcpkg_root or os.path.join(os.getcwd(), 'vcpkg')

    def check_winget(self) -> bool:
        """Check if winget is available."""
        return find_executable('winget') is not None

    def install_winget_package(self, package_id: str, name: str) -> bool:
        """Install a package using winget."""
        print(f"Installing {name}...")
        cmd = ['winget', 'install', '--id', package_id, '-e', '--accept-source-agreements']
        exit_code, _, _ = run_command(cmd, check=False)
        return exit_code == 0

    def setup_vcpkg(self) -> bool:
        """Clone and bootstrap vcpkg."""
        if os.path.isdir(self.vcpkg_root):
            print(f"vcpkg already exists at: {self.vcpkg_root}")
            return True

        print("Cloning vcpkg...")
        cmd = ['git', 'clone', 'https://github.com/microsoft/vcpkg.git', self.vcpkg_root]
        exit_code, _, _ = run_command(cmd, check=False)
        if exit_code != 0:
            return False

        print("Bootstrapping vcpkg...")
        bootstrap = os.path.join(self.vcpkg_root, 'bootstrap-vcpkg.bat')
        exit_code, _, _ = run_command([bootstrap, '-disableMetrics'], check=False)
        return exit_code == 0

    def install_vcpkg_packages(self) -> bool:
        """Install packages using vcpkg."""
        vcpkg_exe = os.path.join(self.vcpkg_root, 'vcpkg.exe')
        if not os.path.isfile(vcpkg_exe):
            print("Error: vcpkg not found")
            return False

        packages = ['zlib:x64-windows']
        for package in packages:
            print(f"Installing {package}...")
            cmd = [vcpkg_exe, 'install', package]
            exit_code, _, _ = run_command(cmd, check=False)
            if exit_code != 0:
                return False

        # Create libz.a symlink for Unix-style linking
        installed_lib = os.path.join(self.vcpkg_root, 'installed', 'x64-windows', 'lib')
        zlib_src = os.path.join(installed_lib, 'zlib.lib')
        zlib_dst = os.path.join(installed_lib, 'libz.a')
        if os.path.isfile(zlib_src) and not os.path.isfile(zlib_dst):
            shutil.copy(zlib_src, zlib_dst)
            print("Created libz.a compatibility link")

        return True

    def install(self, verbose: bool = False) -> bool:
        """Install all Windows dependencies."""
        success = True

        # Install CMake via winget
        if not find_executable('cmake'):
            if self.check_winget():
                if not self.install_winget_package('Kitware.CMake', 'CMake'):
                    print("Warning: Failed to install CMake via winget")
            else:
                print("Warning: winget not available, please install CMake manually")

        # Install LLVM/Clang via winget
        if not find_executable('clang'):
            if self.check_winget():
                # Try LLVM-MinGW first
                self.install_winget_package('mstorsjo.llvm-mingw', 'LLVM-MinGW')
            else:
                print("Warning: winget not available, please install LLVM-MinGW manually")

        # Install Ninja via winget or chocolatey
        if not find_executable('ninja'):
            if self.check_winget():
                self.install_winget_package('Ninja-build.Ninja', 'Ninja')
            elif find_executable('choco'):
                run_command(['choco', 'install', 'ninja', '-y'], check=False)

        # Setup vcpkg and install packages
        if not self.setup_vcpkg():
            print("Warning: Failed to setup vcpkg")
            success = False
        elif not self.install_vcpkg_packages():
            print("Warning: Failed to install vcpkg packages")
            success = False

        return success


class VcpkgInstaller:
    """Install dependencies using vcpkg on any platform."""

    def __init__(self, vcpkg_root: Optional[str] = None):
        if vcpkg_root:
            self.vcpkg_root = vcpkg_root
        else:
            self.vcpkg_root = os.path.join(os.getcwd(), 'vcpkg')

    def get_triplet(self) -> str:
        """Get the vcpkg triplet for the current platform."""
        if is_windows():
            return 'x64-windows'
        elif is_macos():
            return 'x64-osx'
        else:
            return 'x64-linux'

    def setup(self) -> bool:
        """Clone and bootstrap vcpkg."""
        if os.path.isdir(self.vcpkg_root):
            print(f"vcpkg already exists at: {self.vcpkg_root}")
            return True

        print("Cloning vcpkg...")
        cmd = ['git', 'clone', 'https://github.com/microsoft/vcpkg.git', self.vcpkg_root]
        exit_code, _, _ = run_command(cmd, check=False)
        if exit_code != 0:
            return False

        print("Bootstrapping vcpkg...")
        if is_windows():
            bootstrap = os.path.join(self.vcpkg_root, 'bootstrap-vcpkg.bat')
            cmd = [bootstrap, '-disableMetrics']
        else:
            bootstrap = os.path.join(self.vcpkg_root, 'bootstrap-vcpkg.sh')
            cmd = ['sh', bootstrap, '-disableMetrics']

        exit_code, _, _ = run_command(cmd, check=False)
        return exit_code == 0

    def install_packages(self) -> bool:
        """Install packages using vcpkg."""
        if is_windows():
            vcpkg_exe = os.path.join(self.vcpkg_root, 'vcpkg.exe')
        else:
            vcpkg_exe = os.path.join(self.vcpkg_root, 'vcpkg')

        if not os.path.isfile(vcpkg_exe):
            print("Error: vcpkg not found")
            return False

        triplet = self.get_triplet()
        packages = [f'zlib:{triplet}']

        for package in packages:
            print(f"Installing {package}...")
            cmd = [vcpkg_exe, 'install', package]
            exit_code, _, _ = run_command(cmd, check=False)
            if exit_code != 0:
                return False

        return True

    def install(self, verbose: bool = False) -> bool:
        """Full vcpkg setup and package installation."""
        if not self.setup():
            return False
        return self.install_packages()


def main():
    parser = argparse.ArgumentParser(
        description='Cross-platform dependency setup for Sindarin compiler'
    )
    parser.add_argument('--vcpkg', action='store_true',
                       help='Use vcpkg for all platforms')
    parser.add_argument('--system', action='store_true',
                       help='Use system package manager')
    parser.add_argument('--vcpkg-root', help='Path to vcpkg installation')
    parser.add_argument('--check', action='store_true',
                       help='Check dependencies without installing')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Show detailed output')

    args = parser.parse_args()

    print(f"Platform: {platform.system()}")
    print()

    checker = DependencyChecker()

    if args.check:
        # Just check dependencies
        if checker.check_all(args.verbose):
            print("\nAll dependencies are installed!")
            sys.exit(0)
        else:
            print("\nSome dependencies are missing.")
            sys.exit(1)

    # Install dependencies
    print("Installing dependencies...")
    print()

    success = False

    if args.vcpkg or is_windows():
        # Use vcpkg
        installer = VcpkgInstaller(args.vcpkg_root)
        success = installer.install(args.verbose)

        # On Windows, also install build tools
        if is_windows():
            win_installer = WindowsInstaller(args.vcpkg_root)
            # Install CMake, Ninja, Clang if missing
            if not find_executable('cmake'):
                win_installer.install_winget_package('Kitware.CMake', 'CMake')
            if not find_executable('ninja'):
                if find_executable('choco'):
                    run_command(['choco', 'install', 'ninja', '-y'], check=False)
            if not find_executable('clang'):
                win_installer.install_winget_package('mstorsjo.llvm-mingw', 'LLVM-MinGW')

    elif is_macos():
        installer = MacOSInstaller()
        success = installer.install(args.verbose)

    elif is_linux():
        installer = LinuxInstaller()
        success = installer.install(args.verbose)

    else:
        print(f"Unsupported platform: {platform.system()}")
        sys.exit(1)

    print()
    if success:
        print("Dependency installation completed!")

        # Final check
        print()
        if checker.check_all(args.verbose):
            print("\nAll dependencies are now installed!")
            sys.exit(0)
        else:
            print("\nSome dependencies may still need manual installation.")
            sys.exit(1)
    else:
        print("Dependency installation had some issues.")
        sys.exit(1)


if __name__ == '__main__':
    main()
