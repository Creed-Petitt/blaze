#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "Blaze v1.1.0 - Modular Framework"

# Helper to check if a package is installed (Debian/Ubuntu)
is_installed_apt() {
    dpkg -s "$1" &> /dev/null
}

install_dependencies() {
    echo -e "${YELLOW}[+] Checking system dependencies...${NC}"

    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -f /etc/debian_version ]; then
            # Debian/Ubuntu
            CORE_LIBS="cmake g++ build-essential libssl-dev"
            DRIVER_LIBS="libpq-dev libmariadb-dev libhiredis-dev"
            
            MISSING_CORE=""
            for lib in $CORE_LIBS; do
                if ! is_installed_apt $lib; then MISSING_CORE="$MISSING_CORE $lib"; fi
            done

            if [ -n "$MISSING_CORE" ]; then
                echo -e "${YELLOW}[+] Installing Core Dependencies: $MISSING_CORE${NC}"
                sudo apt-get update && sudo apt-get install -y $MISSING_CORE
            fi

            # Always ensure driver headers are present for 'blaze add' to work smoothly
            echo -e "${YELLOW}[+] Ensuring Driver Headers are present...${NC}"
            sudo apt-get install -y $DRIVER_LIBS

        elif [ -f /etc/redhat-release ]; then
            # Fedora / RHEL / Rocky / Alma
            if grep -qE "Rocky|Alma|CentOS" /etc/redhat-release; then
                sudo dnf install -y epel-release
                sudo dnf install -y 'dnf-command(config-manager)'
                sudo dnf config-manager --set-enabled crb
            fi
            sudo dnf install -y gcc-c++ make cmake openssl-devel libpq-devel mariadb-devel hiredis-devel
        
        elif [ -f /etc/arch-release ]; then
            sudo pacman -S --needed cmake openssl postgresql-libs libmariadbclient hiredis
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        if command -v brew &> /dev/null; then
            brew install cmake openssl libpq mariadb-connector-c hiredis
        else
            echo -e "${RED}[!] Homebrew not found. Cannot install dependencies automatically.${NC}"
        fi
    fi
}

# 1. Install system libs
install_dependencies

# 2. Check for Go (Required for CLI build)
if ! command -v go &> /dev/null; then
    echo -e "${RED}Error: Go is not installed. Please install Go (golang.org) to build the CLI.${NC}"
    exit 1
fi

# 3. Handle Remote Install (If user just downloaded the script)
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

# 6. Cleanup
if [ "$REMOTE_INSTALL" = true ]; then
    echo "[+] Cleaning up temporary files..."
    cd /tmp
    rm -rf "$TEMP_DIR"
fi

echo -e "${GREEN}[+] Blaze v1.1 Modular Installation Successful!${NC}"
echo -e "${YELLOW}[!] Type 'blaze init myproject' to start.${NC}"