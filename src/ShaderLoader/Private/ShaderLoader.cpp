//
// Created by charlie on 8/1/25.
//

#include "../Public/ShaderLoader.h"
#include <fstream>
#include <iostream>

namespace ShaderLoader {

    ShaderLoader::ShaderLoader(std::unique_ptr<IShaderCompiler> compiler)
        : m_compiler(std::move(compiler))
    {}

    bool ShaderLoader::loadShader(const std::string& path) {
        // Load SPIR-V directly from file
        auto module = m_compiler->loadSpirvFromFile(path);

        if (module.spirv.empty()) {
            // Log error but don't fail completely
            std::cout << "Failed to load SPIR-V shader: " << module.infoLog << std::endl;
            return false;
        }

        std::cout << module.infoLog << std::endl; // Success message
        m_modules[path] = std::move(module);
        return true;
    }

    const ShaderModule* ShaderLoader::getModule(const std::string& path) const {
        auto it = m_modules.find(path);
        return (it != m_modules.end() ? &it->second : nullptr);
    }

} // namespace ShaderLoader
