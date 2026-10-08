#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
BUILD_TYPE="${BUILD_TYPE:-Release}"

echo "==> Configuring miqutoolkit ($BUILD_TYPE)..."
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="/usr" "$@"

echo "==> Building miqutoolkit..."
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "==> Build finished successfully! Library is in $BUILD_DIR"

# If invoked with sudo/root, automatically install to system
if [ "${EUID}" -eq 0 ]; then
    echo "==> Installing miqutoolkit to system (/usr/lib, /usr/include)..."
    cmake --install "$BUILD_DIR"
    ldconfig || true
    echo "==> miqutoolkit successfully installed to /usr!"
else
    echo "==> Built locally in $BUILD_DIR. To install system-wide, run: sudo ./make.sh"
fi
