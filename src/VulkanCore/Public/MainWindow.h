#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVulkanWindow>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    QVulkanWindow* m_vulkanWindow;

    void loadShader(const QString& filePath, const QString& shaderType);
};

#endif // MAINWINDOW_H
