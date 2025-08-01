#!/bin/bash
set -e

# Get version from debian control file or use default
VERSION=$(grep "^Version:" debian/DEBIAN/control | cut -d' ' -f2 | tr -d ' ')
if [ -z "$VERSION" ]; then
    VERSION="1.0.0"
    echo "Warning: Could not extract version from debian/DEBIAN/control, using default: $VERSION"
else
    echo "Building TinyRequest version: $VERSION"
fi

# 1. Build the app (release mode)
echo "Building release version..."
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --config Release

# 2. Ensure debian directory structure exists
echo "Setting up debian package structure..."
mkdir -p debian/usr/bin
mkdir -p debian/usr/share/tinyrequest/assets/fonts
mkdir -p debian/usr/share/doc/tinyrequest
mkdir -p debian/DEBIAN

# 3. Copy the binary to the debian package structure
echo "Installing binary..."
if [ -f "cmake-build-release/bin/tinyrequest" ]; then
    install -Dm755 cmake-build-release/bin/tinyrequest debian/usr/bin/tinyrequest
    echo "Binary installed successfully"
elif [ -f "cmake-build-release/bin/TinyRequest" ]; then
    install -Dm755 cmake-build-release/bin/TinyRequest debian/usr/bin/tinyrequest
    echo "Binary installed successfully (using TinyRequest -> tinyrequest)"
else
    echo "Error: Binary not found in cmake-build-release/bin/"
    ls -la cmake-build-release/bin/ || true
    exit 1
fi

# 4. Copy assets to the debian package structure
echo "Installing assets..."
mkdir -p debian/usr/share/tinyrequest/assets/fonts
if [ -f "assets/fonts/fa-solid-900.ttf" ]; then
    install -Dm644 assets/fonts/fa-solid-900.ttf debian/usr/share/tinyrequest/assets/fonts/fa-solid-900.ttf
    echo "Font file installed successfully"
else
    echo "Warning: Font file not found at assets/fonts/fa-solid-900.ttf"
fi

# 5. Set proper permissions
echo "Setting permissions..."
if [ "$EUID" -eq 0 ]; then
    # Running as root (e.g., in CI)
    chown root:root debian/DEBIAN/*
    chmod 755 debian/DEBIAN/postinst debian/DEBIAN/postrm 2>/dev/null || true
    chmod 644 debian/DEBIAN/control
else
    # Running as regular user
    sudo chown root:root debian/DEBIAN/*
    sudo chmod 755 debian/DEBIAN/postinst debian/DEBIAN/postrm 2>/dev/null || true
    sudo chmod 644 debian/DEBIAN/control
fi

# 6. Build the .deb package
PACKAGE_NAME="tinyrequest-v${VERSION}.deb"
echo "Building .deb package: $PACKAGE_NAME"

if [ "$EUID" -eq 0 ]; then
    # Running as root (e.g., in CI)
    dpkg-deb --build debian "$PACKAGE_NAME"
else
    # Running as regular user
    sudo dpkg-deb --build debian "$PACKAGE_NAME"
fi

# 7. Verify the package was created
if [ -f "$PACKAGE_NAME" ]; then
    echo "Build and packaging complete: $PACKAGE_NAME"
    echo "Package size: $(du -h "$PACKAGE_NAME" | cut -f1)"
    echo "Package info:"
    dpkg-deb --info "$PACKAGE_NAME" | head -20
else
    echo "Error: Package $PACKAGE_NAME was not created"
    exit 1
fi 