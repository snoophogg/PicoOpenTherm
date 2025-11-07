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
cd "${SCRIPT_DIR}/pico-kvstore/host"
mkdir -p build
cd build
echo "Fetching Pico SDK 2.1.1 for kvstore-util build..."
# First cmake run will fetch the SDK
cmake -DPICO_SDK_FETCH_FROM_GIT=ON \
      -DPICO_SDK_FETCH_FROM_GIT_TAG=2.1.1 \
      -DPICO_SDK_FETCH_FROM_GIT_PATH=./sdk \
      .. || true
# Initialize SDK submodules
if [ -d sdk/pico_sdk-src ]; then
  echo "Initializing SDK submodules..."
  cd sdk/pico_sdk-src
  git submodule update --init --recursive
  cd ../..
fi
# Reconfigure now that submodules are initialized
echo "Configuring kvstore-util build..."
cmake -DPICO_SDK_FETCH_FROM_GIT=ON \
      -DPICO_SDK_FETCH_FROM_GIT_TAG=2.1.1 \
      -DPICO_SDK_FETCH_FROM_GIT_PATH=./sdk \
      ..
make -j$(nproc)
echo -e "${GREEN}✓ kvstore-util built successfully${NC}"
echo "  - pico-kvstore/host/build/kvstore-util"
echo ""

echo "================================================"
echo -e "${GREEN}Build Complete!${NC}"
echo "================================================"
echo ""
echo "Firmware outputs:"
echo "  ${SCRIPT_DIR}/build/picoopentherm.uf2"
echo "  ${SCRIPT_DIR}/build/picoopentherm_simulator.uf2"
echo ""
echo "Tool outputs:"
echo "  ${SCRIPT_DIR}/pico-kvstore/host/build/kvstore-util"
echo ""
echo "Next steps:"
echo "  1. Provision configuration (optional): ./provision-config.sh"
echo "  2. Flash firmware: picotool load build/picoopentherm.uf2"
echo ""
