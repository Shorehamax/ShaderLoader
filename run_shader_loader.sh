#!/bin/bash

# Vulkan Shader Loader Launcher
# This script ensures the app finds the shader files correctly

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Create a temporary symlink to fix the path issue
# The app expects shaders in ../shaders/ but we have them in ./shaders/
cd "$SCRIPT_DIR"

# Create parent directory structure if it doesn't exist
mkdir -p ../

# Create symlink to shaders directory
ln -sf "$SCRIPT_DIR/shaders" "../shaders" 2>/dev/null

# Run the application
echo "🚀 Starting Vulkan Shader Loader..."
echo "📁 Shaders directory: $SCRIPT_DIR/shaders/"
echo "🎨 Edit shaders in the shaders/ folder and recompile to see changes!"
echo ""

./app

# Clean up symlink
rm -f "../shaders" 2>/dev/null
