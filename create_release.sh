#!/bin/bash

# Vulkan Shader Loader - Release Package Creator
# Creates a distributable package with all necessary files

echo "🎨 Creating Shader Loader Release Package..."

# Create release directory
mkdir -p release/ShaderLoader

# Copy essential files - use RELEASE build instead of debug
echo "📁 Copying executable (Release build)..."
cp cmake-build-release/app release/ShaderLoader/

echo "🎨 Copying shader templates..."
cp -r shaders release/ShaderLoader/

echo "📄 Copying documentation..."
cp README.md release/ShaderLoader/

echo "🚀 Copying launcher script..."
cp run_shader_loader.sh release/ShaderLoader/

# Make executables
chmod +x release/ShaderLoader/app
chmod +x release/ShaderLoader/run_shader_loader.sh

# Create archive
echo "📦 Creating distributable archive..."
cd release
tar -czf ShaderLoader-v1.0-linux.tar.gz ShaderLoader/
cd ..

echo "✅ Release package created!"
echo ""
echo "📦 Your release package is ready:"
echo "   📁 release/ShaderLoader/           - Complete package directory"
echo "   📦 release/ShaderLoader-v1.0-linux.tar.gz - Distributable archive"
echo ""
echo "🚀 To distribute:"
echo "   1. Share the .tar.gz file"
echo "   2. Users extract and run: ./app"
echo "   3. Users edit shaders in shaders/ directory"
echo ""
echo "📋 Package contents:"
ls -la release/ShaderLoader/
