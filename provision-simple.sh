#!/bin/bash
# Simple provisioning script for PicoOpenTherm using pre-built tools
#
# This script uses pre-built picotool and kvstore-util binaries from the bin/ folder
# Download these from GitHub releases if you don't want to build from source
#
# Usage:
#   1. Download picotool and kvstore-util from GitHub releases
#   2. Place them in the bin/ folder (make them executable)
#   3. Create secrets.cfg from secrets.cfg.example
#   4. Run this script: ./provision-simple.sh

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SECRETS_FILE="${SCRIPT_DIR}/secrets.cfg"
SETTINGS_BIN="${SCRIPT_DIR}/settings.bin"
BIN_DIR="${SCRIPT_DIR}/bin"

# Flash offset for kvstore (last 256KB of 2MB flash)
FLASH_OFFSET=0x1FC0000
KVSTORE_SIZE=262144  # 256KB

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "================================================"
echo "  PicoOpenTherm Simple Configuration Tool"
echo "================================================"
echo ""

# Check if secrets.cfg exists
if [ ! -f "$SECRETS_FILE" ]; then
    echo -e "${RED}Error: secrets.cfg not found!${NC}"
    echo ""
    echo "Please create secrets.cfg from the template:"
    echo "  cp secrets.cfg.example secrets.cfg"
    echo "  nano secrets.cfg"
    echo ""
    exit 1
fi

echo -e "${GREEN}✓ Found secrets.cfg${NC}"

# Check for bin directory
if [ ! -d "$BIN_DIR" ]; then
    echo -e "${YELLOW}Info: bin/ directory not found, creating it${NC}"
    mkdir -p "$BIN_DIR"
fi

# Look for kvstore-util
KVSTORE_UTIL=""
if [ -f "$BIN_DIR/kvstore-util" ]; then
    KVSTORE_UTIL="$BIN_DIR/kvstore-util"
    chmod +x "$KVSTORE_UTIL" 2>/dev/null || true
elif [ -f "$BIN_DIR/kvstore-util-linux-amd64" ]; then
    KVSTORE_UTIL="$BIN_DIR/kvstore-util-linux-amd64"
    chmod +x "$KVSTORE_UTIL" 2>/dev/null || true
elif [ -f "pico-kvstore/host/build/kvstore-util" ]; then
    KVSTORE_UTIL="pico-kvstore/host/build/kvstore-util"
elif command -v kvstore-util &> /dev/null; then
    KVSTORE_UTIL="kvstore-util"
else
    echo -e "${YELLOW}Warning: kvstore-util not found. Building with compatible SDK version...${NC}"
    echo ""
    
    # Build kvstore-util with SDK 2.1.1 (compatible with its mbedtls usage)
    mkdir -p pico-kvstore/host/build
    cd pico-kvstore/host/build
    
    echo "Fetching Pico SDK 2.1.1 for kvstore-util build..."
    cmake -DPICO_SDK_FETCH_FROM_GIT=ON \
          -DPICO_SDK_FETCH_FROM_GIT_TAG=2.1.1 \
          -DPICO_SDK_FETCH_FROM_GIT_PATH=./sdk \
          .. || {
        echo -e "${RED}Error: CMake configuration failed${NC}"
        echo "Please see KVSTORE-UTIL.md for alternative configuration options."
        exit 1
    }
    
    echo "Building kvstore-util..."
    make || {
        echo -e "${RED}Error: Build failed${NC}"
        echo "Please see KVSTORE-UTIL.md for alternative configuration options."
        exit 1
    }
    
    cd ../../..
    KVSTORE_UTIL="pico-kvstore/host/build/kvstore-util"
    echo -e "${GREEN}✓ kvstore-util built successfully!${NC}"
    echo ""
fi

echo -e "${GREEN}✓ Found kvstore-util${NC}"

# Look for picotool
PICOTOOL_CMD=""
if [ -f "$BIN_DIR/picotool" ]; then
    PICOTOOL_CMD="$BIN_DIR/picotool"
    chmod +x "$PICOTOOL_CMD" 2>/dev/null || true
elif [ -f "$BIN_DIR/picotool-linux-amd64" ]; then
    PICOTOOL_CMD="$BIN_DIR/picotool-linux-amd64"
    chmod +x "$PICOTOOL_CMD" 2>/dev/null || true
elif command -v picotool &> /dev/null; then
    PICOTOOL_CMD="picotool"
else
    echo -e "${RED}Error: picotool not found!${NC}"
    echo ""
    echo "Download it from GitHub releases:"
    echo -e "  ${BLUE}https://github.com/snoophogg/PicoOpenTherm/releases/latest${NC}"
    echo ""
    echo "Then place it in the bin/ folder:"
    echo "  mkdir -p bin"
    echo "  mv ~/Downloads/picotool-linux-amd64-* bin/picotool"
    echo "  chmod +x bin/picotool"
    echo ""
    echo "Or build from source with: ./provision-config.sh"
    echo ""
    exit 1
fi

echo -e "${GREEN}✓ Found picotool${NC}"
echo ""

# Create kvstore binary from secrets.cfg
echo "Creating kvstore binary from secrets.cfg..."
rm -f "$SETTINGS_BIN"

# Read secrets.cfg and create kvstore
while IFS='=' read -r key value || [ -n "$key" ]; do
    # Skip comments and empty lines
    [[ "$key" =~ ^#.*$ ]] && continue
    [[ -z "$key" ]] && continue
    
    # Trim whitespace
    key=$(echo "$key" | xargs)
    value=$(echo "$value" | xargs)
    
    if [ -n "$key" ] && [ -n "$value" ]; then
        echo "  Adding: $key = $value"
        "$KVSTORE_UTIL" "$SETTINGS_BIN" set "$key" "$value"
    fi
done < "$SECRETS_FILE"

echo -e "${GREEN}✓ Created ${SETTINGS_BIN}${NC}"
echo ""

# Check if Pico is connected
echo "Checking for connected Pico W..."
if ! "$PICOTOOL_CMD" info &> /dev/null; then
    echo -e "${YELLOW}Warning: No Pico detected in BOOTSEL mode${NC}"
    echo ""
    echo "Please connect your Pico W in BOOTSEL mode:"
    echo "  1. Hold the BOOTSEL button"
    echo "  2. Connect USB cable"
    echo "  3. Release BOOTSEL button"
    echo ""
    read -p "Press Enter when ready, or Ctrl+C to cancel..."
    
    if ! "$PICOTOOL_CMD" info &> /dev/null; then
        echo -e "${RED}Error: Still no Pico detected${NC}"
        exit 1
    fi
fi

echo -e "${GREEN}✓ Pico W detected${NC}"
echo ""

# Show Pico info
echo "Pico W Information:"
"$PICOTOOL_CMD" info
echo ""

# Flash the kvstore to the device
echo "Flashing configuration to Pico W at offset ${FLASH_OFFSET}..."
echo "  Source: ${SETTINGS_BIN}"
echo "  Offset: ${FLASH_OFFSET} (last 256KB)"
echo "  Size: ${KVSTORE_SIZE} bytes"
echo ""

"$PICOTOOL_CMD" load -t bin "$SETTINGS_BIN" -o "$FLASH_OFFSET"

echo ""
echo -e "${GREEN}✓ Configuration successfully flashed!${NC}"
echo ""
echo "Next steps:"
echo "  1. Download firmware from GitHub releases"
echo "  2. Flash firmware:"
echo "     $PICOTOOL_CMD load picoopentherm-*.uf2"
echo ""
echo "Or copy the UF2 file to the Pico when in BOOTSEL mode."
echo ""
echo "================================================"
