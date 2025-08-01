#!/bin/bash
set -e

echo "Fetching TinyRequest dependencies with exact versions..."

# Create externals directory if it doesn't exist
# Check if we're in scripts directory and go to project root
if [ "$(basename "$PWD")" = "scripts" ]; then
    cd ..
fi
mkdir -p externals
cd externals

# Clone cimgui v1.53.1 with imgui submodule v1.92.1
echo "Fetching cimgui v1.53.1 with imgui v1.92.1..."
if [ ! -d "cimgui" ]; then
  git clone --recursive https://github.com/cimgui/cimgui.git
  cd cimgui
  git checkout v1.53.1
  git submodule update --init --recursive
  # Set imgui submodule to specific version v1.92.1
  cd imgui
  git checkout v1.92.1
  cd ..
  cd ..
  echo "cimgui v1.53.1 with imgui v1.92.1 fetched successfully."
else
  echo "cimgui directory exists. Checking versions..."
  cd cimgui
  current_tag=$(git describe --tags --exact-match 2>/dev/null || echo "unknown")
  if [ "$current_tag" != "v1.53.1" ]; then
    echo "Updating cimgui to v1.53.1..."
    git fetch --all --tags
    git checkout v1.53.1
    git submodule update --init --recursive
  else
    echo "cimgui v1.53.1 already present."
  fi
  
  # Check and update imgui submodule to v1.92.1
  echo "Checking imgui submodule version..."
  cd imgui
  current_imgui_tag=$(git describe --tags --exact-match 2>/dev/null || echo "unknown")
  if [ "$current_imgui_tag" != "v1.92.1" ]; then
    echo "Updating imgui submodule to v1.92.1..."
    git fetch --all --tags
    git checkout v1.92.1
    echo "imgui submodule updated to v1.92.1."
  else
    echo "imgui submodule v1.92.1 already present."
  fi
  cd ..
  cd ..
fi

# Clone cJSON v1.7.15
echo "Fetching cJSON v1.7.15..."
if [ ! -d "cJSON" ]; then
  git clone https://github.com/DaveGamble/cJSON.git
  cd cJSON
  git checkout v1.7.15
  cd ..
  echo "cJSON v1.7.15 fetched successfully."
else
  echo "cJSON directory exists. Checking version..."
  cd cJSON
  current_tag=$(git describe --tags --exact-match 2>/dev/null || echo "unknown")
  if [ "$current_tag" != "v1.7.15" ]; then
    echo "Updating cJSON to v1.7.15..."
    git fetch --all --tags
    git checkout v1.7.15
    echo "cJSON updated to v1.7.15."
  else
    echo "cJSON v1.7.15 already present."
  fi
  cd ..
fi

echo ""
echo "Dependencies fetched successfully!"
echo "- cimgui: v1.53.1"
echo "  └── imgui submodule: v1.92.1"
echo "- cJSON: v1.7.15"
echo ""

# Skip system dependencies if running in GitHub Actions
if [ "$GITHUB_ACTIONS" = "true" ]; then
    echo "Running in GitHub Actions - skipping system dependency installation"
    echo "System dependencies should be installed by the workflow"
    exit 0
fi

# Detect the distribution and install system dependencies
echo "Detecting system distribution..."

if command -v pacman >/dev/null 2>&1; then
    # Arch Linux
    echo "Detected Arch Linux - installing dependencies with pacman..."
    echo "Installing: glfw, curl, cjson, libgl, libx11, libxrandr, libxinerama, libxcursor, libxi"
    sudo pacman -Sy --noconfirm glfw curl cjson libgl libx11 libxrandr libxinerama libxcursor libxi
    echo "Arch Linux dependencies installed successfully!"
    
elif command -v apt >/dev/null 2>&1; then
    # Debian/Ubuntu
    echo "Detected Debian/Ubuntu - installing dependencies with apt..."
    echo "Installing: libglfw3-dev, libcurl4-openssl-dev"
    sudo apt update && sudo apt install -y libglfw3-dev libcurl4-openssl-dev
    echo "Debian/Ubuntu dependencies installed successfully!"
    
elif command -v dnf >/dev/null 2>&1; then
    # Fedora
    echo "Detected Fedora - installing dependencies with dnf..."
    echo "Installing: glfw-devel, libcurl-devel, cjson-devel"
    sudo dnf install -y glfw-devel libcurl-devel cjson-devel mesa-libGL-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel
    echo "Fedora dependencies installed successfully!"
    
elif command -v zypper >/dev/null 2>&1; then
    # openSUSE
    echo "Detected openSUSE - installing dependencies with zypper..."
    echo "Installing: glfw3-devel, libcurl-devel, libcjson-devel"
    sudo zypper install -y glfw3-devel libcurl-devel libcjson-devel Mesa-libGL-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel
    echo "openSUSE dependencies installed successfully!"
    
else
    echo "Warning: Could not detect package manager!"
    echo "Please install the following dependencies manually:"
    echo ""
    echo "For Arch Linux:"
    echo "  sudo pacman -S glfw curl cjson libgl libx11 libxrandr libxinerama libxcursor libxi"
    echo ""
    echo "For Debian/Ubuntu:"
    echo "  sudo apt update && sudo apt install libglfw3-dev libcurl4-openssl-dev"
    echo ""
    echo "For Fedora:"
    echo "  sudo dnf install glfw-devel libcurl-devel cjson-devel mesa-libGL-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel"
    echo ""
    echo "For openSUSE:"
    echo "  sudo zypper install glfw3-devel libcurl-devel libcjson-devel Mesa-libGL-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel"
fi

echo ""
echo "All dependencies processed successfully!"
