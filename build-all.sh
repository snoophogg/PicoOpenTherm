#!/bin/bash
# Build script for PicoOpenTherm firmware and host tools
#
# This script handles the two-stage build process:
# 1. Build firmware (Pico W target)
# 2. Build host tools (kvstore-util)

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "================================================"
echo "  PicoOpenTherm Build Script"
echo "================================================"
echo ""

# Stage 1: Build firmware
echo -e "${BLUE}Stage 1: Building firmware for Pico W...${NC}"
mkdir -p "${SCRIPT_DIR}/build"
cd "${SCRIPT_DIR}/build"
cmake -DPICO_BOARD=pico_w ..
make -j$(nproc)
echo -e "${GREEN}✓ Firmware built successfully${NC}"
echo "  - picoopentherm.uf2"
echo "  - picoopentherm_simulator.uf2"
echo ""

# Stage 2: Build host tools
echo -e "${BLUE}Stage 2: Building host tools (kvstore-util)...${NC}"
export PICO_SDK_PATH="${SCRIPT_DIR}/pico-sdk"
cd "${SCRIPT_DIR}/pico-kvstore/host"
# Apply compatibility patch for Pico SDK 2.2.0+
sed -i.bak '36,39s/^add_library/#add_library/' CMakeLists.txt
mkdir -p build
cd build
cmake ..
make -j$(nproc)
echo -e "${GREEN}✓ Host tools built successfully${NC}"
echo "  - kvstore-util"
echo ""

echo "================================================"
echo -e "${GREEN}Build Complete!${NC}"
echo "================================================"
echo ""
echo "Firmware outputs:"
echo "  ${SCRIPT_DIR}/build/picoopentherm.uf2"
echo "  ${SCRIPT_DIR}/build/picoopentherm_simulator.uf2"
echo ""
echo "Host tools:"
echo "  ${SCRIPT_DIR}/pico-kvstore/host/build/kvstore-util"
echo ""
echo "Next steps:"
echo "  1. Configure: cp secrets.cfg.example secrets.cfg && nano secrets.cfg"
echo "  2. Provision: ./provision-config.sh"
echo "  3. Flash: picotool load build/picoopentherm.uf2"
echo ""
