//
// Created by charlie on 8/1/25.
//

#ifndef SHADERLOADER_H
#define SHADERLOADER_H
#pragma once

#include "IShaderCompiler.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace ShaderLoader {

    class ShaderLoader {
    public:
        explicit ShaderLoader(std::unique_ptr<IShaderCompiler> compiler);

        // Load SPIR-V shader from disk
        // returns true on success
        bool loadShader(const std::string& path);

        // get the compiled SPIR-V module for a previously loaded shader
        const ShaderModule* getModule(const std::string& path) const;

    private:
        std::unique_ptr<IShaderCompiler> m_compiler;
        std::unordered_map<std::string, ShaderModule> m_modules;
    };

} // namespace ShaderLoader

#endif //SHADERLOADER_H
