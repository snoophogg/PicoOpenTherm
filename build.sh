#!/bin/bash
# PicoOpenTherm Build Script
# Builds both hardware and simulator firmware targets

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║     PicoOpenTherm Firmware Build Script       ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════╝${NC}"
echo ""

# Ensure submodules are initialized
if [ ! -f "pico-sdk/CMakeLists.txt" ]; then
    echo -e "${YELLOW}📦 Initializing pico-sdk submodule...${NC}"
    git submodule update --init --recursive
    echo ""
fi

# Parse arguments
BUILD_TARGET=""
CLEAN=false
VERBOSE=false
RECONFIGURE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        hardware|hw)
            BUILD_TARGET="picoopentherm"
            shift
            ;;
        simulator|sim)
            BUILD_TARGET="picoopentherm_simulator"
            shift
            ;;
        both|all)
            BUILD_TARGET="both"
            shift
            ;;
        clean)
            CLEAN=true
            shift
            ;;
        reconfigure|reconfig)
            RECONFIGURE=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            echo "Usage: ./build.sh [TARGET] [OPTIONS]"
            echo ""
            echo "TARGETS:"
            echo "  hardware, hw       Build hardware firmware only (picoopentherm.uf2)"
            echo "  simulator, sim     Build simulator firmware only (picoopentherm_simulator.uf2)"
            echo "  both, all          Build both targets (default)"
            echo ""
            echo "OPTIONS:"
            echo "  clean              Clean build directory before building"
            echo "  reconfigure        Re-run CMake configuration"
            echo "  -v, --verbose      Verbose build output"
            echo "  -h, --help         Show this help message"
            echo ""
            echo "EXAMPLES:"
            echo "  ./build.sh                 # Build both targets"
            echo "  ./build.sh hardware        # Build hardware only"
            echo "  ./build.sh sim clean       # Clean and build simulator"
            echo "  ./build.sh both -v         # Build both with verbose output"
            echo "  ./build.sh reconfigure     # Reconfigure CMake and build"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Use './build.sh --help' for usage information"
            exit 1
            ;;
    esac
done

# Default to building both if no target specified
if [ -z "$BUILD_TARGET" ]; then
    BUILD_TARGET="both"
fi

# Clean build if requested
if [ "$CLEAN" = true ]; then
    echo -e "${YELLOW}🧹 Cleaning build directory...${NC}"
    rm -rf build
    RECONFIGURE=true
    echo ""
fi

# Create build directory and configure if needed
if [ ! -d "build" ] || [ "$RECONFIGURE" = true ]; then
    echo -e "${BLUE}⚙️  Running CMake configuration...${NC}"
    mkdir -p build
    cd build
    cmake -DPICO_BOARD=pico_w ..
    cd ..
    echo -e "${GREEN}✅ Configuration complete!${NC}"
    echo ""
fi

# Build function
build_target() {
    local target=$1
    local name=$2

    echo -e "${BLUE}🔨 Building ${name}...${NC}"

    if [ "$VERBOSE" = true ]; then
        cmake --build build --target "$target" -- -j$(nproc)
    else
        # Show only important output
        OUTPUT=$(cmake --build build --target "$target" -- -j$(nproc) 2>&1)
        EXIT_CODE=$?

        # Show last 10 lines (summary)
        echo "$OUTPUT" | tail -10

        if [ $EXIT_CODE -ne 0 ]; then
            # On error, show more context
            echo ""
            echo -e "${RED}Build errors:${NC}"
            echo "$OUTPUT" | grep -i "error" || echo "$OUTPUT" | tail -20
        fi

        return $EXIT_CODE
    fi
}

# Build targets
SUCCESS=true

if [ "$BUILD_TARGET" = "picoopentherm" ] || [ "$BUILD_TARGET" = "both" ]; then
    if build_target "picoopentherm" "Hardware Firmware (picoopentherm)"; then
        if [ -f "build/picoopentherm.uf2" ]; then
            SIZE=$(ls -lh "build/picoopentherm.uf2" | awk '{print $5}')
            echo -e "${GREEN}✅ Hardware firmware built successfully!${NC}"
            echo -e "   📦 File: build/picoopentherm.uf2 (${SIZE})"
        fi
    else
        echo -e "${RED}❌ Hardware firmware build failed!${NC}"
        SUCCESS=false
    fi
    echo ""
fi

if [ "$BUILD_TARGET" = "picoopentherm_simulator" ] || [ "$BUILD_TARGET" = "both" ]; then
    if build_target "picoopentherm_simulator" "Simulator Firmware (picoopentherm_simulator)"; then
        if [ -f "build/picoopentherm_simulator.uf2" ]; then
            SIZE=$(ls -lh "build/picoopentherm_simulator.uf2" | awk '{print $5}')
            echo -e "${GREEN}✅ Simulator firmware built successfully!${NC}"
            echo -e "   📦 File: build/picoopentherm_simulator.uf2 (${SIZE})"
        fi
    else
        echo -e "${RED}❌ Simulator firmware build failed!${NC}"
        SUCCESS=false
    fi
    echo ""
fi

# Summary
echo -e "${BLUE}═══════════════════════════════════════════════${NC}"
if [ "$SUCCESS" = true ]; then
    echo -e "${GREEN}✨ Build completed successfully!${NC}"
    echo ""

    echo "Firmware files ready:"
    if [ "$BUILD_TARGET" = "picoopentherm" ] || [ "$BUILD_TARGET" = "both" ]; then
        if [ -f "build/picoopentherm.uf2" ]; then
            echo -e "  ${GREEN}•${NC} build/picoopentherm.uf2        (Hardware)"
        fi
    fi
    if [ "$BUILD_TARGET" = "picoopentherm_simulator" ] || [ "$BUILD_TARGET" = "both" ]; then
        if [ -f "build/picoopentherm_simulator.uf2" ]; then
            echo -e "  ${GREEN}•${NC} build/picoopentherm_simulator.uf2 (Simulator)"
        fi
    fi

    echo ""
    echo "To flash firmware to Pico W:"
    echo "  1. Hold BOOTSEL button on Pico W"
    echo "  2. Connect USB cable (appears as RPI-RP2 drive)"
    echo "  3. Copy desired .uf2 file to RPI-RP2 drive"
    echo "  4. Pico W will reboot automatically"
    echo ""
    exit 0
else
    echo -e "${RED}❌ Build failed!${NC}"
    echo ""
    echo "Check error messages above for details."
    if [ "$VERBOSE" = false ]; then
        echo "Run with -v flag for verbose output:"
        echo "  ./build.sh $BUILD_TARGET -v"
    fi
    echo ""
    exit 1
fi
