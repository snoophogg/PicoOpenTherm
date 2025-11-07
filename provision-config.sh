#!/bin/bash
# Provision PicoOpenTherm configuration to device using kvstore-util and picotool
#
# This script:
# 1. Reads configuration from secrets.cfg
# 2. Uses kvstore-util to create a settings.bin file
# 3. Flashes the settings to the Pico W using picotool
#
# Requirements:
# - kvstore-util (from pico-kvstore tools)
# - picotool
# - secrets.cfg file with your configuration

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SECRETS_FILE="${SCRIPT_DIR}/secrets.cfg"
SETTINGS_BIN="${SCRIPT_DIR}/settings.bin"
KVSTORE_UTIL="${SCRIPT_DIR}/pico-kvstore/host/build/kvstore-util"

# Flash offset for kvstore (last 256KB of 2MB flash)
FLASH_OFFSET=0x1FC0000
KVSTORE_SIZE=262144  # 256KB

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "================================================"
echo "  PicoOpenTherm Configuration Provisioning"
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

# Check if kvstore-util exists and is built
if [ ! -f "$KVSTORE_UTIL" ]; then
    echo -e "${YELLOW}Warning: kvstore-util not found at ${KVSTORE_UTIL}${NC}"
    echo ""
    echo "kvstore-util must be built separately due to Pico SDK compatibility issues."
    echo "Please see KVSTORE-UTIL.md for build instructions."
    echo ""
    exit 1
fi

# Check if picotool is available
PICOTOOL_CMD=""

# First check local build
if [ -f "${SCRIPT_DIR}/picotool/build/picotool" ]; then
    PICOTOOL_CMD="${SCRIPT_DIR}/picotool/build/picotool"
    echo -e "${GREEN}✓ Found picotool (local build)${NC}"
elif command -v picotool &> /dev/null; then
    PICOTOOL_CMD="picotool"
    echo -e "${GREEN}✓ Found picotool (system)${NC}"
else
    echo -e "${RED}Error: picotool not found${NC}"
    echo ""
    echo "Please build picotool:"
    echo "  cd picotool"
    echo "  mkdir -p build && cd build"
    echo "  cmake .."
    echo "  make"
    echo ""
    echo "Or install system-wide:"
    echo "  sudo make install"
    echo ""
    exit 1
fi

echo -e "${GREEN}✓ Found kvstore-util${NC}"
echo -e "${GREEN}✓ Found secrets.cfg${NC}"
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
echo "You can now flash your firmware:"
echo "  $PICOTOOL_CMD load build/picoopentherm.uf2"
echo ""
echo "Or copy the UF2 file to the Pico when in BOOTSEL mode."
echo ""
echo "================================================"
