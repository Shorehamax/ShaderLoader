#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Include the existing ShaderLoader system
#include "../ShaderLoader/Public/ShaderLoader.h"
#include "../ShaderLoader/Public/IShaderCompiler.h"

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

class ModernShaderLoaderApp {
public:
    ModernShaderLoaderApp(const std::string& vertShaderPath, const std::string& fragShaderPath, const std::string& computeShaderPath = "")
        : vertPath(vertShaderPath), fragPath(fragShaderPath), computePath(computeShaderPath) {}

    void run() {
        initWindow();
        initVulkan();
        initShaderLoader();
        loadShaders();
        createGraphicsPipeline();
        if (!computePath.empty()) {
            createComputePipeline();
        }
        mainLoop();
        cleanup();
    }

private:
    std::string vertPath, fragPath, computePath;
    GLFWwindow* window = nullptr;
    vk::Instance instance;
    vk::Device device;
    vk::PhysicalDevice physicalDevice;
    vk::Queue graphicsQueue;
    vk::Queue computeQueue;
    uint32_t graphicsQueueFamily = ~0u;
    uint32_t computeQueueFamily = ~0u;
    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::ImageView> swapchainImageViews;
    vk::Format swapchainImageFormat;
    vk::Extent2D swapchainExtent;
    vk::RenderPass renderPass;
    vk::Pipeline graphicsPipeline;
    vk::Pipeline computePipeline;
    vk::PipelineLayout graphicsPipelineLayout;
    vk::PipelineLayout computePipelineLayout;
    std::vector<vk::Framebuffer> framebuffers;
    vk::CommandPool commandPool;
    vk::CommandBuffer commandBuffer;
    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
    vk::Fence inFlightFence;

    std::unique_ptr<ShaderLoader::ShaderLoader> shaderLoader;

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Modern Shader Loader", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapchain();
        createImageViews();
        createRenderPass();
        createFramebuffers();
        createCommandPool();
        createCommandBuffer();
        createSyncObjects();
    }

    void initShaderLoader() {
        // Create a basic shader compiler that loads SPIR-V files
        auto compiler = std::make_unique<BasicShaderCompiler>();
        shaderLoader = std::make_unique<ShaderLoader::ShaderLoader>(std::move(compiler));
    }

    void loadShaders() {
        std::cout << "Loading vertex shader: " << vertPath << std::endl;
        if (!shaderLoader->loadShader(vertPath)) {
            throw std::runtime_error("Failed to load vertex shader");
        }

        std::cout << "Loading fragment shader: " << fragPath << std::endl;
        if (!shaderLoader->loadShader(fragPath)) {
            throw std::runtime_error("Failed to load fragment shader");
        }

        if (!computePath.empty()) {
            std::cout << "Loading compute shader: " << computePath << std::endl;
            if (!shaderLoader->loadShader(computePath)) {
                throw std::runtime_error("Failed to load compute shader");
            }
        }
    }

    void createGraphicsPipeline() {
        auto vertModule = shaderLoader->getModule(vertPath);
        auto fragModule = shaderLoader->getModule(fragPath);

        if (!vertModule || !fragModule) {
            throw std::runtime_error("Failed to get shader modules");
        }

        // Create Vulkan shader modules
        vk::ShaderModuleCreateInfo vertCreateInfo({}, vertModule->spirv.size() * sizeof(uint32_t),
                                                   reinterpret_cast<const uint32_t*>(vertModule->spirv.data()));
        vk::ShaderModuleCreateInfo fragCreateInfo({}, fragModule->spirv.size() * sizeof(uint32_t),
                                                   reinterpret_cast<const uint32_t*>(fragModule->spirv.data()));

        auto vertShaderModule = device.createShaderModule(vertCreateInfo);
        auto fragShaderModule = device.createShaderModule(fragCreateInfo);

        // Create pipeline stages
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {
            {{}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main"},
            {{}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main"}
        };

        // Vertex input (empty for hardcoded vertex shaders)
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

        // Input assembly
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList);

        // Viewport and scissor
        vk::Viewport viewport(0.0f, 0.0f, WIDTH, HEIGHT, 0.0f, 1.0f);
        vk::Rect2D scissor({0, 0}, {WIDTH, HEIGHT});
        vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);

        // Rasterizer
        vk::PipelineRasterizationStateCreateInfo rasterizer({}, false, false, vk::PolygonMode::eFill,
                                                             vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise);

        // Multisampling
        vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1);

        // Color blending
        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        vk::PipelineColorBlendStateCreateInfo colorBlending({}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment);

        // Pipeline layout
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        graphicsPipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

        // Create pipeline
        vk::GraphicsPipelineCreateInfo pipelineInfo({}, shaderStages, &vertexInputInfo, &inputAssembly, nullptr,
                                                     &viewportState, &rasterizer, &multisampling, nullptr,
                                                     &colorBlending, nullptr, graphicsPipelineLayout, renderPass, 0);

        auto result = device.createGraphicsPipeline(nullptr, pipelineInfo);
        graphicsPipeline = result.value;

        // Cleanup
        device.destroyShaderModule(vertShaderModule);
        device.destroyShaderModule(fragShaderModule);

        std::cout << "Graphics pipeline created successfully!" << std::endl;
    }

    void createComputePipeline() {
        auto computeModule = shaderLoader->getModule(computePath);
        if (!computeModule) {
            throw std::runtime_error("Failed to get compute shader module");
        }

        // Create Vulkan shader module
        vk::ShaderModuleCreateInfo createInfo({}, computeModule->spirv.size() * sizeof(uint32_t),
                                              reinterpret_cast<const uint32_t*>(computeModule->spirv.data()));
        auto computeShaderModule = device.createShaderModule(createInfo);

        // Pipeline layout
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        computePipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

        // Create compute pipeline
        vk::ComputePipelineCreateInfo pipelineInfo({}, {{}, vk::ShaderStageFlagBits::eCompute, computeShaderModule, "main"},
                                                    computePipelineLayout);

        auto result = device.createComputePipeline(nullptr, pipelineInfo);
        computePipeline = result.value;

        device.destroyShaderModule(computeShaderModule);

        std::cout << "Compute pipeline created successfully!" << std::endl;
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
        device.waitIdle();
    }

    void drawFrame() {
        device.waitForFences(1, &inFlightFence, VK_TRUE, UINT64_MAX);
        device.resetFences(1, &inFlightFence);

        auto [result, imageIndex] = device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailableSemaphore, nullptr);

        commandBuffer.reset();
        recordCommandBuffer(imageIndex);

        vk::SubmitInfo submitInfo;
        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        submitInfo.setWaitSemaphores(imageAvailableSemaphore);
        submitInfo.setWaitDstStageMask(waitStages);
        submitInfo.setCommandBuffers(commandBuffer);
        submitInfo.setSignalSemaphores(renderFinishedSemaphore);

        graphicsQueue.submit(submitInfo, inFlightFence);

        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphores(renderFinishedSemaphore);
        presentInfo.setSwapchains(swapchain);
        presentInfo.setImageIndices(imageIndex);

        graphicsQueue.presentKHR(presentInfo);
    }

    void recordCommandBuffer(uint32_t imageIndex) {
        vk::CommandBufferBeginInfo beginInfo{};
        commandBuffer.begin(beginInfo);

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffers[imageIndex];
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = swapchainExtent;

        vk::ClearValue clearColor = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
        renderPassInfo.setClearValues(clearColor);

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

        // Draw 6 vertices for hexagon (adjust based on your shader)
        commandBuffer.draw(6, 1, 0, 0);

        commandBuffer.endRenderPass();
        commandBuffer.end();
    }

    // ... (other Vulkan setup methods - createInstance, pickPhysicalDevice, etc.)
    // These would be similar to the existing implementation but simplified

    void cleanup() {
        if (computePipeline) {
            device.destroyPipeline(computePipeline);
            device.destroyPipelineLayout(computePipelineLayout);
        }

        device.destroyPipeline(graphicsPipeline);
        device.destroyPipelineLayout(graphicsPipelineLayout);

        for (auto framebuffer : framebuffers) {
            device.destroyFramebuffer(framebuffer);
        }

        device.destroyRenderPass(renderPass);

        for (auto imageView : swapchainImageViews) {
            device.destroyImageView(imageView);
        }

        device.destroySwapchainKHR(swapchain);
        device.destroyCommandPool(commandPool);
        device.destroySemaphore(imageAvailableSemaphore);
        device.destroySemaphore(renderFinishedSemaphore);
        device.destroyFence(inFlightFence);
        device.destroy();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    // Simplified Vulkan setup methods would go here...
};

// Basic implementation of IShaderCompiler for loading SPIR-V files
class BasicShaderCompiler : public ShaderLoader::IShaderCompiler {
public:
    ShaderLoader::ShaderModule loadSpirvFromFile(const std::string& path) override {
        ShaderLoader::ShaderModule module;

        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            module.infoLog = "Failed to open file: " + path;
            return module;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        module.spirv.resize(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(module.spirv.data()), fileSize);
        file.close();

        module.infoLog = "Successfully loaded: " + path;
        return module;
    }
};

int main(int argc, char* argv[]) {
    try {
        std::string vertShader = "../shaders/custom_vertex.vert.spv";
        std::string fragShader = "../shaders/custom_fragment.frag.spv";
        std::string computeShader = "";

        // Command line arguments for shader selection
        if (argc >= 3) {
            vertShader = argv[1];
            fragShader = argv[2];
        }
        if (argc >= 4) {
            computeShader = argv[3];
        }

        std::cout << "Starting Modern Shader Loader App" << std::endl;
        std::cout << "Vertex Shader: " << vertShader << std::endl;
        std::cout << "Fragment Shader: " << fragShader << std::endl;
        if (!computeShader.empty()) {
            std::cout << "Compute Shader: " << computeShader << std::endl;
        }

        ModernShaderLoaderApp app(vertShader, fragShader, computeShader);
        app.run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
