//
// Created by charlie on 8/2/25.
//

#ifndef QTVULKANWINDOW_H
#define QTVULKANWINDOW_H


#include <QVulkanWindow>

namespace VulkanCore {

    class QtVulkanWindow : public QVulkanWindow {
    public:
        QVulkanWindowRenderer* createRenderer() override;

        // Shader loading interface
        bool loadShadersFromFiles(const QString& vertexShaderPath, const QString& fragmentShaderPath);

    private:
        class VulkanWindowRenderer* m_renderer = nullptr;
    };

} // namespace VulkanCore

#endif //QTVULKANWINDOW_H
