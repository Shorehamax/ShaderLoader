//
// Created by charlie on 8/1/25.
//

#include "../Public/IShaderCompiler.h"
#include <memory>
#include <fstream>
#include <iostream>
#include <cstring>

namespace ShaderLoader {

    class ShaderCompiler : public IShaderCompiler {
    public:
        ShaderModule loadSpirvFromFile(const std::string& path) override {
            std::ifstream file(path, std::ios::binary);
            if (!file) {
                return {.spirv = {}, .infoLog = "Failed to open SPIR-V file: " + path};
            }

            // Get file size
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);

            // Check if file is empty
            if (size == 0) {
                return {.spirv = {}, .infoLog = "SPIR-V file is empty: " + path};
            }

            // Check if file size is valid (must be multiple of 4 bytes)
            if (size % sizeof(uint32_t) != 0) {
                return {.spirv = {}, .infoLog = "Invalid SPIR-V file size (not multiple of 4 bytes): " + path + ", size: " + std::to_string(size)};
            }

            // Read SPIR-V data using stream iterators
            std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            size = buffer.size();
            if (size == 0) {
                return {.spirv = {}, .infoLog = "SPIR-V file is empty: " + path};
            }
            if (size % sizeof(uint32_t) != 0) {
                return {.spirv = {}, .infoLog = "Invalid SPIR-V file size (not multiple of 4 bytes): " + path + ", size: " + std::to_string(size)};
            }
            // Copy bytes into uint32_t vector
            size_t wordCount = size / sizeof(uint32_t);
            std::vector<uint32_t> spirvData(wordCount);
            memcpy(spirvData.data(), buffer.data(), size);

            // Basic SPIR-V validation - check magic number
            if (spirvData.empty()) {
                return {.spirv = {}, .infoLog = "SPIR-V data is empty after reading: " + path};
            }
            if (spirvData[0] != 0x07230203) {
                return {.spirv = {}, .infoLog = "Invalid SPIR-V magic number in file: " + path +
                    ", expected: 0x07230203, got: 0x" + std::to_string(spirvData[0])};
            }

            std::string info = "Successfully loaded SPIR-V from: " + path +
                " (size: " + std::to_string(size) + " bytes, " + std::to_string(wordCount) + " words)";
            return { std::move(spirvData), std::move(info) };
        }

        ShaderModule loadDynamicShader(const std::string& path) {
            std::ifstream file(path, std::ios::binary);
            if (!file) {
                return {.spirv = {}, .infoLog = "Failed to open shader file: " + path};
            }

            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);

            if (size == 0) {
                return {.spirv = {}, .infoLog = "Shader file is empty: " + path};
            }

            if (size % sizeof(uint32_t) != 0) {
                return {.spirv = {}, .infoLog = "Invalid shader file size (not multiple of 4 bytes): " + path};
            }

            std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            size_t wordCount = size / sizeof(uint32_t);
            std::vector<uint32_t> spirvData(wordCount);
            memcpy(spirvData.data(), buffer.data(), size);

            if (spirvData.empty() || spirvData[0] != 0x07230203) {
                return {.spirv = {}, .infoLog = "Invalid SPIR-V magic number in shader file: " + path};
            }

            return {std::move(spirvData), "Successfully loaded shader: " + path};
        }
    };

    // Factory function to get the default compiler
    std::unique_ptr<IShaderCompiler> createDefaultCompiler() {
        return std::make_unique<ShaderCompiler>();
    }

} // namespace ShaderLoader
