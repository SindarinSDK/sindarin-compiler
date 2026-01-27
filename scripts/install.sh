#!/bin/bash
#
# Sindarin Compiler Universal Installer
# Detects OS flavor and installs the appropriate package from GitHub releases.
#
# Supported platforms:
#   - macOS (via Homebrew)
#   - Debian/Ubuntu (via .deb)
#   - RedHat/Fedora/CentOS (via .rpm)
#   - Arch Linux (via PKGBUILD)
#

set -e

REPO="SindarinSDK/sindarin-compiler"
VERSION="v0.0.7-alpha"
VERSION_NUM="0.0.7"
GITHUB_RELEASE_URL="https://github.com/${REPO}/releases/download/${VERSION}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Detect the operating system
detect_os() {
    local os=""
    local distro=""

    case "$(uname -s)" in
        Darwin)
            os="macos"
            ;;
        Linux)
            os="linux"
            if [ -f /etc/os-release ]; then
                . /etc/os-release
                case "$ID" in
                    debian|ubuntu|linuxmint|pop|elementary|zorin)
                        distro="debian"
                        ;;
                    fedora|rhel|centos|rocky|alma|ol)
                        distro="redhat"
                        ;;
                    arch|manjaro|endeavouros|garuda)
                        distro="arch"
                        ;;
                    *)
                        # Check ID_LIKE for derivative distributions
                        case "$ID_LIKE" in
                            *debian*|*ubuntu*)
                                distro="debian"
                                ;;
                            *rhel*|*fedora*|*centos*)
                                distro="redhat"
                                ;;
                            *arch*)
                                distro="arch"
                                ;;
                            *)
                                distro="unknown"
                                ;;
                        esac
                        ;;
                esac
            else
                distro="unknown"
            fi
            ;;
        *)
            os="unknown"
            ;;
    esac

    echo "${os}:${distro}"
}

# Create a temporary directory for downloads
create_temp_dir() {
    TEMP_DIR=$(mktemp -d)
    trap "rm -rf ${TEMP_DIR}" EXIT
    print_info "Created temporary directory: ${TEMP_DIR}"
}

# Download a file from GitHub releases
# Returns the path to the downloaded file via stdout
download_file() {
    local filename="$1"
    local url="${GITHUB_RELEASE_URL}/${filename}"
    local dest="${TEMP_DIR}/${filename}"

    print_info "Downloading ${filename}..." >&2

    if command_exists curl; then
        curl -fsSL -o "${dest}" "${url}"
    elif command_exists wget; then
        wget -q -O "${dest}" "${url}"
    else
        print_error "Neither curl nor wget found. Please install one of them." >&2
        exit 1
    fi

    if [ ! -f "${dest}" ]; then
        print_error "Failed to download ${filename}" >&2
        exit 1
    fi

    print_success "Downloaded ${filename}" >&2
    echo "${dest}"
}

# Install on macOS using Homebrew
install_macos() {
    print_info "Detected macOS"

    if ! command_exists brew; then
        print_error "Homebrew is not installed. Please install Homebrew first:"
        echo "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        exit 1
    fi

    local formula_path
    formula_path=$(download_file "sindarin.rb")

    print_info "Installing via Homebrew..."
    brew install --formula "${formula_path}"

    print_success "Sindarin compiler installed successfully!"
}

# Install on Debian/Ubuntu using .deb
install_debian() {
    print_info "Detected Debian/Ubuntu-based distribution"

    local deb_file="sindarin_${VERSION_NUM}_amd64.deb"
    local deb_path
    deb_path=$(download_file "${deb_file}")

    print_info "Installing .deb package..."

    if command_exists apt; then
        sudo apt install -y "${deb_path}"
    elif command_exists dpkg; then
        sudo dpkg -i "${deb_path}"
        # Install any missing dependencies
        sudo apt-get install -f -y 2>/dev/null || true
    else
        print_error "Neither apt nor dpkg found. Cannot install .deb package."
        exit 1
    fi

    print_success "Sindarin compiler installed successfully!"
}

# Install on RedHat/Fedora/CentOS using .rpm
install_redhat() {
    print_info "Detected RedHat/Fedora/CentOS-based distribution"

    local rpm_file="sindarin-${VERSION_NUM}-1.x86_64.rpm"
    local rpm_path
    rpm_path=$(download_file "${rpm_file}")

    print_info "Installing .rpm package..."

    if command_exists dnf; then
        sudo dnf install -y "${rpm_path}"
    elif command_exists yum; then
        sudo yum install -y "${rpm_path}"
    elif command_exists rpm; then
        sudo rpm -i "${rpm_path}"
    else
        print_error "Neither dnf, yum, nor rpm found. Cannot install .rpm package."
        exit 1
    fi

    print_success "Sindarin compiler installed successfully!"
}

# Install on Arch Linux using PKGBUILD
install_arch() {
    print_info "Detected Arch Linux-based distribution"

    if ! command_exists makepkg; then
        print_error "makepkg not found. Please ensure base-devel is installed."
        exit 1
    fi

    local pkgbuild_path
    pkgbuild_path=$(download_file "PKGBUILD")

    print_info "Building and installing via makepkg..."

    # Move to temp directory and build
    local build_dir="${TEMP_DIR}/build"
    mkdir -p "${build_dir}"
    cp "${pkgbuild_path}" "${build_dir}/"

    (
        cd "${build_dir}"
        makepkg -si --noconfirm
    )

    print_success "Sindarin compiler installed successfully!"
}

# Fallback: install from tarball
install_tarball() {
    local os_type="$1"
    local tarball=""
    local install_dir="/usr/local"

    case "$os_type" in
        macos)
            tarball="sindarin-${VERSION_NUM}-macos-x64.tar.gz"
            ;;
        linux)
            tarball="sindarin-${VERSION_NUM}-linux-x64.tar.gz"
            ;;
        *)
            print_error "Unsupported operating system"
            exit 1
            ;;
    esac

    print_warning "Falling back to tarball installation"

    local tarball_path
    tarball_path=$(download_file "${tarball}")

    print_info "Extracting to ${install_dir}..."

    sudo tar -xzf "${tarball_path}" -C "${install_dir}" --strip-components=1

    print_success "Sindarin compiler installed to ${install_dir}"
    print_info "Make sure ${install_dir}/bin is in your PATH"
}

# Print usage information
print_usage() {
    echo "Sindarin Compiler Installer"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help       Show this help message"
    echo "  -v, --version    Show version to be installed"
    echo "  --tarball        Force tarball installation instead of package manager"
    echo ""
    echo "Supported platforms:"
    echo "  - macOS (via Homebrew)"
    echo "  - Debian/Ubuntu (via .deb)"
    echo "  - RedHat/Fedora/CentOS (via .rpm)"
    echo "  - Arch Linux (via PKGBUILD)"
}

# Main function
main() {
    local force_tarball=false

    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -h|--help)
                print_usage
                exit 0
                ;;
            -v|--version)
                echo "Sindarin Compiler Installer - Version ${VERSION}"
                exit 0
                ;;
            --tarball)
                force_tarball=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                print_usage
                exit 1
                ;;
        esac
    done

    echo "========================================"
    echo "  Sindarin Compiler Installer ${VERSION}"
    echo "========================================"
    echo ""

    # Detect OS
    local os_info
    os_info=$(detect_os)
    local os_type="${os_info%%:*}"
    local distro="${os_info##*:}"

    print_info "Detected OS: ${os_type}, Distribution: ${distro:-N/A}"

    # Create temp directory for downloads
    create_temp_dir

    # Handle forced tarball installation
    if [ "$force_tarball" = true ]; then
        install_tarball "$os_type"
        exit 0
    fi

    # Install based on detected OS
    case "$os_type" in
        macos)
            install_macos
            ;;
        linux)
            case "$distro" in
                debian)
                    install_debian
                    ;;
                redhat)
                    install_redhat
                    ;;
                arch)
                    install_arch
                    ;;
                *)
                    print_warning "Unknown Linux distribution: ${distro}"
                    print_info "Attempting tarball installation..."
                    install_tarball "linux"
                    ;;
            esac
            ;;
        *)
            print_error "Unsupported operating system: ${os_type}"
            exit 1
            ;;
    esac

    echo ""
    print_info "Verifying installation..."
    if command_exists sn; then
        print_success "Sindarin compiler is now available!"
        echo ""
        sn --version 2>/dev/null || echo "Run 'sn --help' to get started"
    else
        print_warning "Installation completed, but 'sn' command not found in PATH"
        print_info "You may need to restart your shell or add the install directory to PATH"
    fi
}

main "$@"
