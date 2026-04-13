#!/bin/bash
#
# Sindarin Prerequisites Installer (Linux / macOS)
#
# Installs build tools required to compile Sindarin programs.
# Supports: Ubuntu, Debian, Fedora, RHEL, CentOS, Rocky, AlmaLinux,
#           Arch, Manjaro, Alpine, openSUSE, Void Linux, and macOS.
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/SindarinSDK/sindarin-compiler/main/scripts/install-prereqs.sh | bash
#   bash scripts/install-prereqs.sh
#

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
WHITE='\033[1;37m'
NC='\033[0m'

write_status() {
    local message="$1"
    local type="${2:-Info}"
    case "$type" in
        Info)    echo -e "${CYAN}[*]${NC} $message" ;;
        Success) echo -e "${GREEN}[+]${NC} $message" ;;
        Warning) echo -e "${YELLOW}[!]${NC} $message" ;;
        Error)   echo -e "${RED}[-]${NC} $message" ;;
    esac
}

install_apt() {
    write_status "Installing build-essential via apt-get..."
    sudo apt-get update -y
    sudo apt-get install -y build-essential cmake ninja-build pkg-config curl zip unzip tar
}

install_dnf_devtools() {
    write_status "Installing Development Tools via dnf..."
    sudo dnf groupinstall -y "Development Tools"
    sudo dnf install -y cmake ninja-build pkgconf curl zip unzip tar
}

install_yum_devtools() {
    write_status "Installing Development Tools via yum..."
    sudo yum groupinstall -y "Development Tools"
    sudo yum install -y cmake ninja-build pkgconfig curl zip unzip tar
}

install_pacman() {
    write_status "Installing base-devel via pacman..."
    sudo pacman -S --needed --noconfirm base-devel cmake ninja pkgconf curl zip unzip tar
}

install_apk() {
    write_status "Installing build-base via apk..."
    if command -v sudo >/dev/null 2>&1; then
        sudo apk add --no-cache build-base cmake samurai pkgconf curl zip unzip tar
    else
        apk add --no-cache build-base cmake samurai pkgconf curl zip unzip tar
    fi
}

install_zypper() {
    write_status "Installing devel_basis pattern via zypper..."
    sudo zypper install -y -t pattern devel_basis
    sudo zypper install -y cmake ninja pkgconf-pkg-config curl zip unzip tar
}

install_xbps() {
    write_status "Installing build tools via xbps-install..."
    sudo xbps-install -Sy gcc make glibc-devel binutils cmake ninja pkgconf curl zip unzip tar
}

install_linux() {
    if [ ! -f /etc/os-release ]; then
        write_status "Cannot detect Linux distribution: /etc/os-release not found" "Error"
        exit 1
    fi

    # shellcheck source=/dev/null
    . /etc/os-release

    write_status "Detected: ${PRETTY_NAME:-$ID}"

    case "$ID" in
        ubuntu|debian|linuxmint|pop)
            install_apt ;;
        fedora)
            install_dnf_devtools ;;
        rhel|centos|rocky|almalinux)
            if command -v dnf >/dev/null 2>&1; then
                install_dnf_devtools
            else
                install_yum_devtools
            fi
            ;;
        arch|manjaro|endeavouros|garuda)
            install_pacman ;;
        alpine)
            install_apk ;;
        opensuse-leap|opensuse-tumbleweed|opensuse)
            install_zypper ;;
        void)
            install_xbps ;;
        *)
            # Fall back to ID_LIKE for unrecognised derivatives
            case "${ID_LIKE:-}" in
                *debian*|*ubuntu*)
                    write_status "Unrecognised distro '$ID', treating as Debian-family (ID_LIKE=$ID_LIKE)" "Warning"
                    install_apt ;;
                *rhel*|*fedora*|*centos*)
                    write_status "Unrecognised distro '$ID', treating as RHEL-family (ID_LIKE=$ID_LIKE)" "Warning"
                    if command -v dnf >/dev/null 2>&1; then
                        install_dnf_devtools
                    else
                        install_yum_devtools
                    fi
                    ;;
                *arch*)
                    write_status "Unrecognised distro '$ID', treating as Arch-family (ID_LIKE=$ID_LIKE)" "Warning"
                    install_pacman ;;
                *suse*)
                    write_status "Unrecognised distro '$ID', treating as openSUSE-family (ID_LIKE=$ID_LIKE)" "Warning"
                    install_zypper ;;
                *)
                    write_status "Unsupported Linux distribution: $ID" "Error"
                    write_status "Please install GCC and make manually for your distribution." "Error"
                    exit 1
                    ;;
            esac
            ;;
    esac
}

install_macos() {
    write_status "Detected macOS"

    if ! command -v brew >/dev/null 2>&1; then
        write_status "Homebrew is not installed." "Error"
        echo ""
        echo -e "${WHITE}  Homebrew is required to install Sindarin prerequisites on macOS.${NC}"
        echo -e "${WHITE}  To install Homebrew, run:${NC}"
        echo ""
        echo -e "${YELLOW}    /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"${NC}"
        echo ""
        echo -e "${WHITE}  Or visit: https://brew.sh${NC}"
        echo ""
        echo -e "${WHITE}  Then re-run this script.${NC}"
        echo ""
        exit 1
    fi

    write_status "Installing coreutils via Homebrew..."
    brew install coreutils cmake ninja pkg-config
}

install_vcpkg() {
    local vcpkg_root="$HOME/vcpkg"

    if [ -d "$vcpkg_root" ] && [ -x "$vcpkg_root/vcpkg" ]; then
        write_status "vcpkg already installed at $vcpkg_root" "Success"
    else
        write_status "Installing vcpkg to $vcpkg_root..."
        git clone https://github.com/microsoft/vcpkg.git "$vcpkg_root"
        "$vcpkg_root/bootstrap-vcpkg.sh" -disableMetrics
        write_status "vcpkg installed" "Success"
    fi

    # Export for current session
    export VCPKG_ROOT="$vcpkg_root"
    export PATH="$vcpkg_root:$PATH"

    # Register in GITHUB_ENV/GITHUB_PATH when running in GitHub Actions
    if [ -n "${GITHUB_ENV:-}" ]; then
        echo "VCPKG_ROOT=$vcpkg_root" >> "$GITHUB_ENV"
    fi
    if [ -n "${GITHUB_PATH:-}" ]; then
        echo "$vcpkg_root" >> "$GITHUB_PATH"
    fi
}

main() {
    echo ""
    echo -e "${MAGENTA}========================================${NC}"
    echo -e "${MAGENTA}  Sindarin Prerequisites Installer${NC}"
    echo -e "${MAGENTA}========================================${NC}"
    echo ""

    case "$(uname -s)" in
        Linux)  install_linux ;;
        Darwin) install_macos ;;
        *)
            write_status "Unsupported operating system: $(uname -s)" "Error"
            exit 1
            ;;
    esac

    install_vcpkg

    echo ""
    write_status "Prerequisites installed successfully!" "Success"
    echo ""
}

main "$@"
