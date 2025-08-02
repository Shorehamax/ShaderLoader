#include "MainWindow.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QVulkanWindow>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    // Create Vulkan instance
    QVulkanInstance* vulkanInstance = new QVulkanInstance();
    vulkanInstance->setApiVersion(QVersionNumber(1, 1, 0));
    if (!vulkanInstance->create()) {
        qFatal("Failed to create Vulkan instance");
    }

    // Set Vulkan instance for QVulkanWindow
    m_vulkanWindow = new QVulkanWindow();
    m_vulkanWindow->setVulkanInstance(vulkanInstance);

    // Set up the central widget and layout
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    // Add Vulkan rendering window
    QWidget* vulkanWidget = QWidget::createWindowContainer(m_vulkanWindow);
    layout->addWidget(vulkanWidget);

    // Add shader selection dropdown
    QComboBox* shaderDropdown = new QComboBox(this);
    shaderDropdown->addItem("Vertex Shader");
    shaderDropdown->addItem("Fragment Shader");
    layout->addWidget(shaderDropdown);

    // Add load shader button
    QPushButton* loadShaderButton = new QPushButton("Load Shader", this);
    layout->addWidget(loadShaderButton);

    // Connect button click to shader loading
    connect(loadShaderButton, &QPushButton::clicked, this, [this, shaderDropdown]() {
        QString filePath = QFileDialog::getOpenFileName(this, "Select Shader File", "./shaders", "SPIR-V Files (*.spv)");
        if (!filePath.isEmpty()) {
            loadShader(filePath, shaderDropdown->currentText());
        }
    });

    setCentralWidget(centralWidget);
}

void MainWindow::loadShader(const QString& filePath, const QString& shaderType) {
    if (shaderType == "Vertex Shader") {
        // Load vertex shader logic
        qDebug() << "Loading vertex shader from: " << filePath;
        // ... Vulkan logic to apply vertex shader ...
    } else if (shaderType == "Fragment Shader") {
        // Load fragment shader logic
        qDebug() << "Loading fragment shader from: " << filePath;
        // ... Vulkan logic to apply fragment shader ...
    }
}
