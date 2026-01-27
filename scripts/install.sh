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
GITHUB_API_URL="https://api.github.com/repos/${REPO}/releases/latest"

# These will be set by fetch_latest_release()
VERSION=""
VERSION_NUM=""
GITHUB_RELEASE_URL=""

# Colors for output (matching PowerShell style)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${CYAN}[*]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[+]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_error() {
    echo -e "${RED}[-]${NC} $1"
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Fetch the latest release version from GitHub API
fetch_latest_release() {
    print_info "Fetching latest release from GitHub..."

    local response=""

    if command_exists curl; then
        response=$(curl -fsSL -H "Accept: application/vnd.github.v3+json" "${GITHUB_API_URL}")
    elif command_exists wget; then
        response=$(wget -q -O - --header="Accept: application/vnd.github.v3+json" "${GITHUB_API_URL}")
    else
        print_error "Neither curl nor wget found. Please install one of them."
        exit 1
    fi

    # Extract tag_name (e.g., "v0.0.12-alpha")
    VERSION=$(echo "${response}" | grep -o '"tag_name"[[:space:]]*:[[:space:]]*"[^"]*"' | head -1 | sed 's/.*"tag_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/')

    if [ -z "${VERSION}" ]; then
        print_error "Failed to fetch latest release version"
        exit 1
    fi

    # Extract version number (e.g., "0.0.12" from "v0.0.12-alpha")
    VERSION_NUM=$(echo "${VERSION}" | sed 's/^v//' | sed 's/-.*$//')

    # Set the download URL
    GITHUB_RELEASE_URL="https://github.com/${REPO}/releases/download/${VERSION}"

    print_success "Found release: ${VERSION}"
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

    if ! command_exists dpkg; then
        print_error "dpkg not found. Cannot install .deb package."
        exit 1
    fi

    # Use dpkg directly to avoid triggering configuration of unrelated broken packages
    sudo dpkg -i "${deb_path}"

    # Install any missing dependencies
    if command_exists apt-get; then
        sudo apt-get install -f -y 2>/dev/null || true
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

# Test the installation by compiling and running a hello world program
test_installation() {
    print_info "Testing Sindarin installation..."

    local test_dir="${TEMP_DIR}/test"
    local test_file="${test_dir}/hello.sn"
    local test_exe="${test_dir}/hello"

    mkdir -p "${test_dir}"

    # Write hello world program
    cat > "${test_file}" << 'EOF'
fn main(): int =>
    println("Hello, World from Sindarin!")
    return 0
EOF

    print_info "Created test file: ${test_file}"

    # Compile the test program
    print_info "Compiling hello.sn..."
    if ! timeout 30 sn "${test_file}" -o "${test_exe}" 2>&1; then
        print_error "Compilation failed"
        return 1
    fi

    # Verify the executable was created
    if [ ! -f "${test_exe}" ]; then
        print_error "Compilation succeeded but no executable was created"
        return 1
    fi

    print_success "Compilation successful!"

    # Run the compiled program
    print_info "Running hello..."
    local output
    if ! output=$(timeout 10 "${test_exe}" 2>&1); then
        print_error "Execution failed"
        return 1
    fi

    echo ""
    echo -e "${WHITE}  Output: ${output}${NC}"
    echo ""

    # Verify output contains expected text
    if echo "${output}" | grep -q "Hello.*World.*Sindarin"; then
        print_success "Test passed! Sindarin is working correctly."
        return 0
    else
        print_warning "Test completed but output was unexpected"
        return 1
    fi
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
    echo "  --skip-test      Skip the hello world compilation test"
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
    local skip_test=false

    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -h|--help)
                print_usage
                exit 0
                ;;
            -v|--version)
                fetch_latest_release
                echo "Sindarin Compiler Installer - will install ${VERSION}"
                exit 0
                ;;
            --tarball)
                force_tarball=true
                shift
                ;;
            --skip-test)
                skip_test=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                print_usage
                exit 1
                ;;
        esac
    done

    echo ""
    echo -e "${MAGENTA}========================================${NC}"
    echo -e "${MAGENTA}       Sindarin SDK Installer${NC}"
    echo -e "${MAGENTA}========================================${NC}"
    echo ""

    # Fetch latest release version from GitHub
    fetch_latest_release

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

        # Run hello world test unless skipped
        if [ "$skip_test" = false ]; then
            echo ""
            if ! test_installation; then
                print_error "Installation completed but tests failed"
                exit 1
            fi
        fi

        echo ""
        print_success "Sindarin SDK installation complete!"
        echo ""
        echo -e "${WHITE}  You can now use 'sn' to compile Sindarin programs.${NC}"
        echo -e "${WHITE}  Example: sn myprogram.sn -o myprogram${NC}"
        echo ""
    else
        print_warning "Installation completed, but 'sn' command not found in PATH"
        print_info "You may need to restart your shell or add the install directory to PATH"
        exit 1
    fi
}

main "$@"
