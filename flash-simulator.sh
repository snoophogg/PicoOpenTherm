#!/bin/bash
# Flash the simulator firmware and provision it with configuration
#
# Usage: ./flash-simulator.sh

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SECRETS_FILE="${SCRIPT_DIR}/secrets.cfg"
SETTINGS_BIN="${SCRIPT_DIR}/settings.bin"
BIN_DIR="${SCRIPT_DIR}/bin"
SIMULATOR_UF2="${BIN_DIR}/picoopentherm_simulator.uf2"

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
echo "  PicoOpenTherm Simulator Flash Tool"
echo "================================================"
echo ""

# Check if simulator firmware exists
if [ ! -f "$SIMULATOR_UF2" ]; then
    echo -e "${RED}Error: Simulator firmware not found!${NC}"
    echo ""
    echo "Expected location: ${SIMULATOR_UF2}"
    echo ""
    echo "Please build the simulator first:"
    echo "  ./build-all.sh"
    echo ""
    exit 1
fi

echo -e "${GREEN}✓ Found simulator firmware${NC}"

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

# Look for kvstore-util
KVSTORE_UTIL=""
if [ -f "$BIN_DIR/kvstore-util" ]; then
    KVSTORE_UTIL="$BIN_DIR/kvstore-util"
    chmod +x "$KVSTORE_UTIL" 2>/dev/null || true
elif [ -f "pico-kvstore/host/build/kvstore-util" ]; then
    KVSTORE_UTIL="pico-kvstore/host/build/kvstore-util"
else
    echo -e "${RED}Error: kvstore-util not found!${NC}"
    echo ""
    echo "Please build it first:"
    echo "  ./provision-config.sh"
    echo ""
    exit 1
fi

echo -e "${GREEN}✓ Found kvstore-util${NC}"

# Look for picotool
PICOTOOL_CMD=""
if [ -f "$BIN_DIR/picotool" ]; then
    PICOTOOL_CMD="$BIN_DIR/picotool"
    chmod +x "$PICOTOOL_CMD" 2>/dev/null || true
elif command -v picotool &> /dev/null; then
    PICOTOOL_CMD="picotool"
else
    echo -e "${RED}Error: picotool not found!${NC}"
    echo ""
    echo "Please install picotool or run: ./provision-config.sh"
    echo ""
    exit 1
fi

echo -e "${GREEN}✓ Found picotool${NC}"
echo ""

# Create kvstore binary from secrets.cfg
echo "Creating kvstore binary from secrets.cfg..."
rm -f "$SETTINGS_BIN"

# First create the kvstore file
"$KVSTORE_UTIL" create -f "$SETTINGS_BIN" -s "$KVSTORE_SIZE"

# Read secrets.cfg and populate kvstore
while IFS='=' read -r key value || [ -n "$key" ]; do
    # Skip comments and empty lines
    [[ "$key" =~ ^#.*$ ]] && continue
    [[ -z "$key" ]] && continue
    
    # Trim whitespace
    key=$(echo "$key" | xargs)
    value=$(echo "$value" | xargs)
    
    if [ -n "$key" ] && [ -n "$value" ]; then
        echo "  Adding: $key = $value"
        "$KVSTORE_UTIL" set -f "$SETTINGS_BIN" -k "$key" -v "$value"
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

# Flash the simulator firmware
echo "Flashing simulator firmware..."
echo "  Source: ${SIMULATOR_UF2}"
echo ""

"$PICOTOOL_CMD" load "$SIMULATOR_UF2"

echo ""
echo -e "${GREEN}✓ Simulator firmware flashed!${NC}"
echo ""

# Wait for device to reconnect
echo "Waiting for device to restart (5 seconds)..."
sleep 5

# Flash the kvstore configuration
echo "Flashing configuration to Pico W at offset ${FLASH_OFFSET}..."
echo "  Source: ${SETTINGS_BIN}"
echo "  Offset: ${FLASH_OFFSET} (last 256KB)"
echo "  Size: ${KVSTORE_SIZE} bytes"
echo ""

# Put device back in BOOTSEL mode
echo -e "${YELLOW}Please put the Pico back in BOOTSEL mode:${NC}"
echo "  1. Disconnect USB"
echo "  2. Hold BOOTSEL button"
echo "  3. Reconnect USB"
echo "  4. Release BOOTSEL"
echo ""
read -p "Press Enter when ready..."

if ! "$PICOTOOL_CMD" info &> /dev/null; then
    echo -e "${RED}Error: Pico not detected${NC}"
    exit 1
fi

"$PICOTOOL_CMD" load -t bin "$SETTINGS_BIN" -o "$FLASH_OFFSET"

echo ""
echo -e "${GREEN}✓ Configuration successfully flashed!${NC}"
echo ""
echo "================================================"
echo "  Simulator is ready!"
echo "================================================"
echo ""
echo "The Pico W will now:"
echo "  • Connect to WiFi: $(grep '^wifi.ssid' $SECRETS_FILE | cut -d= -f2 | xargs)"
echo "  • Connect to MQTT broker"
echo "  • Publish simulated OpenTherm data"
echo "  • Appear in Home Assistant as 'OpenTherm Gateway'"
echo ""
echo "Check your Home Assistant MQTT integration!"
echo ""
