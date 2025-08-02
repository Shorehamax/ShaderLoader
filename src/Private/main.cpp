#include <iostream>
#include <QApplication>
#include <QVulkanInstance>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include "../VulkanCore/Public/QtVulkanWindow.h"
#include "../VulkanCore/Public/MainWindow.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // First, let the user select SPIR-V shader files
    QString vertexShaderPath = QFileDialog::getOpenFileName(
        nullptr,
        "Select Vertex Shader (SPIR-V)",
        QDir::homePath(),
        "SPIR-V Files (*.spv);;All Files (*)"
    );

    if (vertexShaderPath.isEmpty()) {
        QMessageBox::information(nullptr, "Info", "No vertex shader selected. Using default shaders.");
    }

    QString fragmentShaderPath;
    if (!vertexShaderPath.isEmpty()) {
        fragmentShaderPath = QFileDialog::getOpenFileName(
            nullptr,
            "Select Fragment Shader (SPIR-V)",
            QFileInfo(vertexShaderPath).absolutePath(),
            "SPIR-V Files (*.spv);;All Files (*)"
        );

        if (fragmentShaderPath.isEmpty()) {
            QMessageBox::information(nullptr, "Info", "No fragment shader selected. Using default shaders.");
        }
    }

    QVulkanInstance inst;
    // Set the API version to avoid the "apiVersion has value of 0" error
    inst.setApiVersion(QVersionNumber(1, 1, 0));
    if (!inst.create()) {
        qFatal("Failed to create Vulkan instance via Qt");
        return -1;
    }

    VulkanCore::QtVulkanWindow window;
    window.setVulkanInstance(&inst);
    window.setTitle("Shader Playground - SPIR-V Loader");
    window.resize(800, 600);

    // Load custom shaders if both were selected
    if (!vertexShaderPath.isEmpty() && !fragmentShaderPath.isEmpty()) {
        if (!window.loadShadersFromFiles(vertexShaderPath, fragmentShaderPath)) {
            QMessageBox::warning(nullptr, "Error", "Failed to load selected SPIR-V shaders. Using default shaders instead.");
        }
    }

    window.show();

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
