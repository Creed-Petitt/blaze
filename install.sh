#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}--- Blaze CLI Installer ---${NC}"

# Helper to check if a package is installed (Debian/Ubuntu)
is_installed_apt() {
    dpkg -s "$1" &> /dev/null
}

install_dependencies() {
    echo -e "${YELLOW}[?] Do you want to check and install system dependencies (OpenSSL, Drivers)? [y/N]${NC}"
    
    # Read from /dev/tty to handle 'curl | bash' scenarios
    if [ -t 0 ]; then
        read -r response
    else
        read -r response < /dev/tty
    fi

    if [[ ! "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
        return
    fi

    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -f /etc/debian_version ]; then
            # Debian/Ubuntu
            echo "[+] Detected Debian/Ubuntu..."
            MISSING=""
            if ! is_installed_apt cmake; then MISSING="$MISSING cmake"; fi
            if ! is_installed_apt libssl-dev; then MISSING="$MISSING libssl-dev"; fi
            if ! is_installed_apt libpq-dev; then MISSING="$MISSING libpq-dev"; fi
            if ! is_installed_apt libmysqlclient-dev; then MISSING="$MISSING libmysqlclient-dev"; fi

            if [ -n "$MISSING" ]; then
                echo -e "${YELLOW}[+] Installing missing packages: $MISSING${NC}"
                sudo apt-get update && sudo apt-get install -y $MISSING
            else
                echo -e "${GREEN}[+] All dependencies satisfied.${NC}"
            fi

        elif [ -f /etc/redhat-release ]; then
            # Fedora / RHEL
            echo "[+] Detected Fedora/RHEL..."
            sudo dnf install -y cmake openssl-devel libpq-devel mysql-devel
        
        elif [ -f /etc/arch-release ]; then
            # Arch Linux
            echo "[+] Detected Arch Linux..."
            sudo pacman -S --needed cmake openssl postgresql-libs libmariadbclient
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        if command -v brew &> /dev/null; then
            echo "[+] Detected macOS (Homebrew)..."
            brew install cmake openssl libpq mysql-client
        else
            echo -e "${RED}[!] Homebrew not found. Skipping dependency check.${NC}"
        fi
    fi
}

# Dependency Check (Optional)
install_dependencies

# Check for Go (Required for CLI build)
if ! command -v go &> /dev/null; then
    echo -e "${RED}Error: Go is not installed. Please install Go (golang.org) to build the CLI.${NC}"
    exit 1
fi

# Handle Remote Install
if [ ! -d "cli" ]; then
    echo "[+] Remote install detected. Cloning Blaze repository..."
    TEMP_DIR=$(mktemp -d)
    git clone --depth 1 https://github.com/Creed-Petitt/blaze.git "$TEMP_DIR"
    cd "$TEMP_DIR"
    REMOTE_INSTALL=true
fi

# Build
echo "[+] Building Blaze CLI..."
if [ -d "cli" ]; then
    cd cli
    go build -o ../blaze_bin
    cd ..
else
    echo -e "${RED}Error: 'cli' directory not found.${NC}"
    exit 1
fi

# Install
echo "[+] Installing binary to /usr/local/bin..."
if [ -w /usr/local/bin ]; then
    mv blaze_bin /usr/local/bin/blaze
else
    sudo mv blaze_bin /usr/local/bin/blaze
fi

# Cleanup
if [ "$REMOTE_INSTALL" = true ]; then
    echo "[+] Cleaning up temporary files..."
    cd /tmp
    rm -rf "$TEMP_DIR"
fi

echo -e "${GREEN}[+] Installation Successful!${NC}"
echo -e "${YELLOW}[!] Type 'blaze init myproject' to start.${NC}"
