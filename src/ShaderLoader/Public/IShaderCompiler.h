//
// Created by charlie on 8/1/25.
//

#ifndef ISHADERCOMPILER_H
#define ISHADERCOMPILER_H
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <memory>

namespace ShaderLoader {

    enum class ShaderLanguage {
        GLSL,
        HLSL
    };

    struct ShaderModule {
        std::vector<uint32_t> spirv;
        std::string           infoLog;
    };

    class IShaderCompiler {
    public:
        virtual ~IShaderCompiler() = default;

        // Load SPIR-V directly from file
        virtual ShaderModule loadSpirvFromFile(const std::string& path) = 0;
    };

    // Factory function to create the default compiler
    std::unique_ptr<IShaderCompiler> createDefaultCompiler();

}

#endif
