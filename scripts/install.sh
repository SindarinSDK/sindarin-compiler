#!/bin/bash
#
# Sindarin Compiler Installer
#
# This script:
#   1. Downloads the latest release from GitHub
#   2. Extracts it to ~/.sn
#   3. Adds ~/.sn/bin to the user PATH (if not already present)
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/SindarinSDK/sindarin-compiler/main/scripts/install.sh | bash
#

set -e

REPO_OWNER="SindarinSDK"
REPO_NAME="sindarin-compiler"
INSTALL_DIR="$HOME/.sn"
BIN_DIR="$INSTALL_DIR/bin"

# Optional auth token — avoids GitHub API rate limits (60/hr unauthenticated).
# Set GH_TOKEN or GITHUB_TOKEN in the environment to authenticate.
# GitHub Actions sets GH_TOKEN automatically when passed via env:.
setup_auth() {
    TOKEN="${GH_TOKEN:-${GITHUB_TOKEN:-}}"
    CURL_AUTH=()
    WGET_AUTH=()
    if [ -n "$TOKEN" ]; then
        CURL_AUTH=(-H "Authorization: Bearer $TOKEN")
        WGET_AUTH=(--header="Authorization: Bearer $TOKEN")
    fi
}

# Colors for output
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

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Detect operating system
detect_os() {
    case "$(uname -s)" in
        Darwin)
            echo "darwin"
            ;;
        Linux)
            echo "linux"
            ;;
        *)
            write_status "Unsupported operating system: $(uname -s)" "Error"
            exit 1
            ;;
    esac
}

# Detect CPU architecture
detect_arch() {
    case "$(uname -m)" in
        x86_64|amd64)
            echo "x64"
            ;;
        aarch64|arm64)
            echo "arm64"
            ;;
        *)
            write_status "Unsupported architecture: $(uname -m)" "Error"
            exit 1
            ;;
    esac
}

# Run the prerequisites installer (build tools, compilers, etc.)
run_prereqs() {
    local prereqs_url="https://raw.githubusercontent.com/${REPO_OWNER}/${REPO_NAME}/main/scripts/install-prereqs.sh"
    local local_script=""

    if [ -n "${BASH_SOURCE[0]:-}" ]; then
        local_script="$(dirname "${BASH_SOURCE[0]}")/install-prereqs.sh"
    fi

    if [ -n "$local_script" ] && [ -f "$local_script" ]; then
        write_status "Running prerequisites installer from $local_script..."
        bash "$local_script"
    else
        write_status "Fetching prerequisites installer..."
        local prereqs_tmp
        prereqs_tmp=$(mktemp)

        if command_exists curl; then
            curl -fsSL "${CURL_AUTH[@]}" -o "$prereqs_tmp" "$prereqs_url"
        elif command_exists wget; then
            wget -q -O "$prereqs_tmp" "${WGET_AUTH[@]}" "$prereqs_url"
        else
            rm -f "$prereqs_tmp"
            write_status "Neither curl nor wget found. Please install one of them." "Error"
            exit 1
        fi

        bash "$prereqs_tmp"
        rm -f "$prereqs_tmp"
    fi
}

# Fetch the latest release from GitHub
get_latest_release() {
    write_status "Fetching latest release from GitHub..."

    local api_url="https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/releases/latest"
    local response=""

    if command_exists curl; then
        response=$(curl -fsSL "${CURL_AUTH[@]}" -H "Accept: application/vnd.github.v3+json" -H "User-Agent: Sindarin-Installer" "$api_url")
    elif command_exists wget; then
        response=$(wget -q -O - "${WGET_AUTH[@]}" --header="Accept: application/vnd.github.v3+json" --header="User-Agent: Sindarin-Installer" "$api_url")
    else
        write_status "Neither curl nor wget found. Please install one of them." "Error"
        exit 1
    fi

    # Extract tag_name
    TAG_NAME=$(echo "$response" | grep -o '"tag_name"[[:space:]]*:[[:space:]]*"[^"]*"' | head -1 | sed 's/.*"tag_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/')

    if [ -z "$TAG_NAME" ]; then
        write_status "Failed to fetch latest release version" "Error"
        exit 1
    fi

    write_status "Found release: $TAG_NAME" "Success"
}

# Download and install Sindarin
install_sindarin() {
    local os_type="$1"

    # Extract version number from tag (e.g., "0.0.12" from "v0.0.12-alpha")
    local version_num
    version_num=$(echo "$TAG_NAME" | sed 's/^v//' | sed 's/-.*$//')

    # Determine the tarball name
    local arch_type="$2"
    local tarball_name="sindarin-${version_num}-${os_type}-${arch_type}.tar.gz"
    local download_url="https://github.com/${REPO_OWNER}/${REPO_NAME}/releases/download/${TAG_NAME}/${tarball_name}"

    # Create temp directory
    local temp_dir
    temp_dir=$(mktemp -d)
    trap "rm -rf '$temp_dir'" EXIT

    local tarball_path="${temp_dir}/${tarball_name}"

    # Download the release tarball
    write_status "Downloading ${tarball_name}..."

    if command_exists curl; then
        curl -fsSL "${CURL_AUTH[@]}" -o "$tarball_path" "$download_url"
    elif command_exists wget; then
        wget -q -O "$tarball_path" "${WGET_AUTH[@]}" "$download_url"
    fi

    if [ ! -f "$tarball_path" ]; then
        write_status "Failed to download ${tarball_name}" "Error"
        exit 1
    fi

    write_status "Download complete" "Success"

    # Remove existing installation if present
    if [ -d "$INSTALL_DIR" ]; then
        write_status "Removing existing installation at $INSTALL_DIR..."
        rm -rf "$INSTALL_DIR"
    fi

    # Create installation directory
    mkdir -p "$INSTALL_DIR"

    # Extract the tarball
    write_status "Extracting to $INSTALL_DIR..."
    tar -xzf "$tarball_path" -C "$INSTALL_DIR"

    # The tarball might contain a nested folder - flatten if needed
    local nested_dirs
    nested_dirs=$(find "$INSTALL_DIR" -mindepth 1 -maxdepth 1 -type d)
    local nested_count
    nested_count=$(echo "$nested_dirs" | grep -c . || true)

    if [ "$nested_count" -eq 1 ] && [ -d "${nested_dirs}/bin" ]; then
        write_status "Flattening nested directory structure..."
        mv "${nested_dirs}"/* "$INSTALL_DIR/" 2>/dev/null || true
        mv "${nested_dirs}"/.[!.]* "$INSTALL_DIR/" 2>/dev/null || true
        rmdir "$nested_dirs"
    fi

    # Verify bin directory exists
    if [ ! -d "$BIN_DIR" ]; then
        write_status "Installation failed: bin directory not found at $BIN_DIR" "Error"
        exit 1
    fi

    # Verify sn exists
    if [ ! -f "$BIN_DIR/sn" ]; then
        write_status "Installation failed: sn not found at $BIN_DIR/sn" "Error"
        exit 1
    fi

    # Make sn executable
    chmod +x "$BIN_DIR/sn"

    write_status "Extraction complete" "Success"
}

# Add to PATH in shell configuration
add_to_path() {
    write_status "Configuring PATH..."

    # In GitHub Actions, write to GITHUB_PATH so the bin dir is available
    # to all subsequent steps without needing a shell restart.
    if [ -n "${GITHUB_PATH:-}" ]; then
        write_status "GitHub Actions detected, registering in GITHUB_PATH..."
        echo "$BIN_DIR" >> "$GITHUB_PATH"
        write_status "Added $BIN_DIR to GITHUB_PATH" "Success"
        return
    fi

    local path_line="export PATH=\"\$HOME/.sn/bin:\$PATH\""
    local path_added=false

    # Determine which shell config files to update
    local shell_configs=()

    # Always check for bash
    if [ -f "$HOME/.bashrc" ]; then
        shell_configs+=("$HOME/.bashrc")
    fi
    if [ -f "$HOME/.bash_profile" ]; then
        shell_configs+=("$HOME/.bash_profile")
    elif [ -f "$HOME/.profile" ]; then
        shell_configs+=("$HOME/.profile")
    fi

    # Check for zsh
    if [ -f "$HOME/.zshrc" ]; then
        shell_configs+=("$HOME/.zshrc")
    fi

    # If no config files found, create .profile
    if [ ${#shell_configs[@]} -eq 0 ]; then
        shell_configs+=("$HOME/.profile")
        touch "$HOME/.profile"
    fi

    for config_file in "${shell_configs[@]}"; do
        # Check if PATH entry already exists (various forms)
        if grep -qE '(^|\s)export\s+PATH=.*\.sn/bin' "$config_file" 2>/dev/null || \
           grep -qE '(^|\s)PATH=.*\.sn/bin' "$config_file" 2>/dev/null; then
            write_status "PATH already configured in $config_file" "Success"
        else
            # Add the path line
            echo "" >> "$config_file"
            echo "# Sindarin compiler" >> "$config_file"
            echo "$path_line" >> "$config_file"
            write_status "Added PATH to $config_file" "Success"
            path_added=true
        fi
    done

    # Handle fish shell separately
    if [ -d "$HOME/.config/fish" ]; then
        local fish_config="$HOME/.config/fish/config.fish"
        local fish_path_line="set -gx PATH \$HOME/.sn/bin \$PATH"

        if [ -f "$fish_config" ] && grep -q '\.sn/bin' "$fish_config" 2>/dev/null; then
            write_status "PATH already configured in $fish_config" "Success"
        else
            mkdir -p "$HOME/.config/fish"
            echo "" >> "$fish_config"
            echo "# Sindarin compiler" >> "$fish_config"
            echo "$fish_path_line" >> "$fish_config"
            write_status "Added PATH to $fish_config" "Success"
            path_added=true
        fi
    fi
}

main() {
    echo ""
    echo -e "${MAGENTA}========================================${NC}"
    echo -e "${MAGENTA}    Sindarin Compiler Installer${NC}"
    echo -e "${MAGENTA}========================================${NC}"
    echo ""

    # Step 1: Set up optional auth headers
    setup_auth

    # Step 2: Detect OS and architecture
    local os_type
    os_type=$(detect_os)
    local arch_type
    arch_type=$(detect_arch)
    write_status "Detected OS: $os_type ($arch_type)"

    # Step 3: Install prerequisites (build tools)
    run_prereqs

    # Step 4: Get latest release info
    get_latest_release

    # Step 5: Download and install
    install_sindarin "$os_type" "$arch_type"

    # Step 6: Add to PATH (idempotent - won't duplicate)
    add_to_path

    echo ""
    write_status "Sindarin compiler installed successfully!" "Success"
    echo ""
    echo -e "${WHITE}  Installed to: $INSTALL_DIR${NC}"
    echo -e "${WHITE}  Version:      $TAG_NAME${NC}"
    echo ""
    echo -e "${YELLOW}  ============================================${NC}"
    echo -e "${YELLOW}  IMPORTANT: Please restart your terminal${NC}"
    echo -e "${YELLOW}  for PATH changes to take effect.${NC}"
    echo -e "${YELLOW}  ============================================${NC}"
    echo ""
    echo -e "${WHITE}  After restarting, you can compile Sindarin programs:${NC}"
    echo -e "${WHITE}    sn myprogram.sn -o myprogram${NC}"
    echo ""
}

main "$@"
