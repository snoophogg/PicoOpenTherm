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
echo -e "${YELLOW}Note: kvstore-util build skipped due to Pico SDK 2.2.0 incompatibility${NC}"
echo -e "${YELLOW}See KVSTORE-UTIL.md for alternative build instructions${NC}"
echo ""

echo "================================================"
echo -e "${GREEN}Build Complete!${NC}"
echo "================================================"
echo ""
echo "Firmware outputs:"
echo "  ${SCRIPT_DIR}/build/picoopentherm.uf2"
echo "  ${SCRIPT_DIR}/build/picoopentherm_simulator.uf2"
echo ""
echo "Note: kvstore-util build was skipped."
echo "See KVSTORE-UTIL.md for build instructions if needed."
echo ""
echo "Next steps:"
echo "  1. Flash firmware: picotool load build/picoopentherm.uf2"
echo "  2. For configuration provisioning, see KVSTORE-UTIL.md"
echo ""
