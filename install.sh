#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "Blaze v1.0.0 - High Performance C++ Framework"

# Helper to check if a package is installed (Debian/Ubuntu)
is_installed_apt() {
    dpkg -s "$1" &> /dev/null
}

install_dependencies() {
    echo -e "${YELLOW}[+] Checking system dependencies...${NC}"

    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -f /etc/debian_version ]; then
            # Debian/Ubuntu
            CORE_LIBS="cmake g++ build-essential libssl-dev ccache pkg-config"
            DRIVER_LIBS="libpq-dev libmariadb-dev libhiredis-dev"
            
            MISSING_CORE=""
            for lib in $CORE_LIBS; do
                if ! is_installed_apt $lib; then MISSING_CORE="$MISSING_CORE $lib"; fi
            done

            if [ -n "$MISSING_CORE" ]; then
                echo -e "${YELLOW}[+] Installing Core Dependencies: $MISSING_CORE${NC}"
                sudo apt-get update && sudo apt-get install -y $MISSING_CORE
            fi

            echo -e "${YELLOW}[+] Ensuring Driver Headers are present...${NC}"
            sudo apt-get install -y $DRIVER_LIBS

        elif [ -f /etc/redhat-release ]; then
            # Fedora / RHEL / Rocky / Alma
            echo -e "${YELLOW}[+] Detected RHEL/Fedora-based system${NC}"
            
            # Enable EPEL/CRB for RHEL derivatives if needed
            if grep -qE "Rocky|Alma|CentOS" /etc/redhat-release; then
                sudo dnf install -y epel-release
                sudo dnf config-manager --set-enabled crb || true
            fi

            # Fedora/RHEL package names often differ
            sudo dnf install -y gcc-c++ make cmake openssl-devel libpq-devel mariadb-devel hiredis-devel ccache pkg-config
        
        elif [ -f /etc/arch-release ]; then
            # Arch Linux
            echo -e "${YELLOW}[+] Detected Arch Linux${NC}"
            sudo pacman -S --noconfirm --needed base-devel cmake openssl postgresql-libs mariadb-libs hiredis ccache
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        # MacOS
        if ! command -v brew &> /dev/null; then
            echo -e "${RED}[!] Homebrew not found. Please install Homebrew first.${NC}"
            exit 1
        fi
        echo -e "${YELLOW}[+] Detected MacOS. Installing via Homebrew...${NC}"
        brew install cmake openssl libpq mariadb-connector-c hiredis ccache pkg-config
    else
        echo -e "${RED}[!] Unsupported OS. Please install dependencies manually.${NC}"
    fi
}

# 1. Install system libs + ccache
install_dependencies

# 2. Check for Go
if ! command -v go &> /dev/null; then
    echo -e "${RED}Error: Go is not installed. Please install Go (golang.org) to build the CLI.${NC}"
    exit 1
fi

# 3. Handle Remote Install
REMOTE_INSTALL=false
if [ ! -d "cli" ]; then
    echo -e "${YELLOW}[+] Remote install detected. Cloning Blaze repository...${NC}"
    TEMP_DIR=$(mktemp -d)
    git clone --depth 1 https://github.com/Creed-Petitt/blaze.git "$TEMP_DIR"
    cd "$TEMP_DIR"
    REMOTE_INSTALL=true
fi

# 4. Build Blaze CLI
echo "[+] Building Blaze CLI..."
if [ -d "cli" ]; then
    cd cli
    go mod tidy
    go build -o ../blaze_bin
    cd ..
else
    echo -e "${RED}Error: 'cli' directory not found.${NC}"
    exit 1
fi

# 5. Install to system path
echo "[+] Installing binary to /usr/local/bin..."
if [ -w /usr/local/bin ]; then
    mv blaze_bin /usr/local/bin/blaze
else
    sudo mv blaze_bin /usr/local/bin/blaze
fi

# 6. Setup Blaze Cache Directories
echo "[+] Initializing global cache..."
mkdir -p ~/.blaze/cache

# 7. Cleanup
if [ "$REMOTE_INSTALL" = true ]; then
    echo "[+] Cleaning up temporary files..."
    cd /tmp
    rm -rf "$TEMP_DIR"
fi

echo -e "${GREEN}[+] Blaze v1.0.0 Installation Successful!${NC}"
echo -e "${YELLOW}[!] Type 'blaze init myproject' to start.${NC}"