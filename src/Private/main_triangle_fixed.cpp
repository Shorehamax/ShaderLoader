#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <algorithm>
#include <limits>
#include <array>

// Use traditional Vulkan-Hpp headers without RAII
#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
    }

    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription( 0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos) ),
            vk::VertexInputAttributeDescription( 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) )
        };
    }
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow *                     window = nullptr;
    vk::Instance                     instance;
    vk::DebugUtilsMessengerEXT       debugMessenger;
    vk::SurfaceKHR                   surface;
    vk::PhysicalDevice               physicalDevice;
    vk::Device                       device;
    uint32_t                         queueIndex = ~0;
    vk::Queue                        queue;

    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat = vk::Format::eUndefined;
    vk::Extent2D swapChainExtent;
    std::vector<vk::ImageView> swapChainImageViews;

    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;
    vk::RenderPass renderPass;
    std::vector<vk::Framebuffer> swapChainFramebuffers;

    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexBufferMemory;

    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;

    std::vector<vk::Semaphore> presentCompleteSemaphore;
    std::vector<vk::Semaphore> renderFinishedSemaphore;
    std::vector<vk::Fence> inFlightFences;
    uint32_t semaphoreIndex = 0;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;

    std::vector<const char*> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName
    };

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = static_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();
        createIndexBuffer();
        createCommandBuffers();
        createSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        device.waitIdle();
    }

    void cleanupSwapChain() {
        for (auto& framebuffer : swapChainFramebuffers) {
            device.destroyFramebuffer(framebuffer);
        }

        for (auto& imageView : swapChainImageViews) {
            device.destroyImageView(imageView);
        }
        device.destroySwapchainKHR(swapChain);
    }

    void cleanup() {
        cleanupSwapChain();

        // Fix: Destroy semaphores with correct loop count
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            device.destroySemaphore(renderFinishedSemaphore[i]);
            device.destroySemaphore(presentCompleteSemaphore[i]);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            device.destroyFence(inFlightFences[i]);
        }

        device.destroyCommandPool(commandPool);
        device.destroyPipeline(graphicsPipeline);
        device.destroyPipelineLayout(pipelineLayout);
        device.destroyRenderPass(renderPass);
        device.destroy();

        // Skip debug messenger cleanup since we disabled it
        // if (enableValidationLayers) {
        //     instance.destroyDebugUtilsMessengerEXT(debugMessenger);
        // }

        instance.destroySurfaceKHR(surface);
        instance.destroy();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        device.waitIdle();

        cleanupSwapChain();
        createSwapChain();
        createImageViews();
        createFramebuffers();
    }

    void createInstance() {
        vk::ApplicationInfo appInfo(
            "Hello Triangle",
            VK_MAKE_VERSION(1, 0, 0),
            "No Engine",
            VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_0
        );

        // Get the required extensions.
        auto requiredExtensions = getRequiredExtensions();

        vk::InstanceCreateInfo createInfo(
            {},
            &appInfo,
            enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
            enableValidationLayers ? validationLayers.data() : nullptr,
            static_cast<uint32_t>(requiredExtensions.size()),
            requiredExtensions.data()
        );

        instance = vk::createInstance(createInfo);
    }

    void setupDebugMessenger() {
        // Disable debug messenger for now to avoid linker issues
        // if (!enableValidationLayers) return;
        return; // Skip debug messenger setup
    }

    void createSurface() {
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(static_cast<VkInstance>(instance), window, nullptr, &_surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        surface = vk::SurfaceKHR(_surface);
    }

    void pickPhysicalDevice() {
        std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (!physicalDevice) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    bool isDeviceSuitable(vk::PhysicalDevice device) {
        auto queueFamilies = device.getQueueFamilyProperties();

        bool graphicsQueueFound = false;
        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                if (device.getSurfaceSupportKHR(i, surface)) {
                    queueIndex = i;
                    graphicsQueueFound = true;
                    break;
                }
            }
        }

        return graphicsQueueFound;
    }

    void createLogicalDevice() {
        float queuePriority = 1.0f;
        vk::DeviceQueueCreateInfo queueCreateInfo(
            {},
            queueIndex,
            1,
            &queuePriority
        );

        vk::PhysicalDeviceFeatures deviceFeatures{};

        vk::DeviceCreateInfo createInfo(
            {},
            1,
            &queueCreateInfo,
            enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
            enableValidationLayers ? validationLayers.data() : nullptr,
            static_cast<uint32_t>(requiredDeviceExtension.size()),
            requiredDeviceExtension.data(),
            &deviceFeatures
        );

        device = physicalDevice.createDevice(createInfo);
        queue = device.getQueue(queueIndex, 0);
    }

    void createSwapChain() {
        auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        swapChainImageFormat = chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
        swapChainExtent = chooseSwapExtent(surfaceCapabilities);

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
            imageCount = surfaceCapabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo(
            {},
            surface,
            imageCount,
            swapChainImageFormat,
            vk::ColorSpaceKHR::eSrgbNonlinear,
            swapChainExtent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::SharingMode::eExclusive,
            0,
            nullptr,
            surfaceCapabilities.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            chooseSwapPresentMode(physicalDevice.getSurfacePresentModesKHR(surface)),
            VK_TRUE
        );

        swapChain = device.createSwapchainKHR(createInfo);
        swapChainImages = device.getSwapchainImagesKHR(swapChain);
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vk::ImageViewCreateInfo createInfo(
                {},
                swapChainImages[i],
                vk::ImageViewType::e2D,
                swapChainImageFormat,
                {},
                { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
            );

            swapChainImageViews[i] = device.createImageView(createInfo);
        }
    }

    void createRenderPass() {
        vk::AttachmentDescription colorAttachment(
            {},
            swapChainImageFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR
        );

        vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

        vk::SubpassDescription subpass(
            {},                           // flags
            vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
            {},                           // inputAttachments (empty)
            colorAttachmentRef,           // colorAttachments
            {},                           // resolveAttachments (empty)
            nullptr,                      // depthStencilAttachment
            {}                            // preserveAttachments (empty)
        );

        vk::RenderPassCreateInfo renderPassInfo(
            {},
            1,
            &colorAttachment,
            1,
            &subpass
        );

        renderPass = device.createRenderPass(renderPassInfo);
    }

    void createGraphicsPipeline() {
        // Load custom vertex and fragment shaders - users can easily edit these!
        auto vertShaderCode = readFile("../shaders/custom_vertex.vert.spv");
        auto fragShaderCode = readFile("../shaders/custom_fragment.frag.spv");

        vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo(
            {},
            vk::ShaderStageFlagBits::eVertex,
            vertShaderModule,
            "main"
        );

        vk::PipelineShaderStageCreateInfo fragShaderStageInfo(
            {},
            vk::ShaderStageFlagBits::eFragment,
            fragShaderModule,
            "main"
        );

        vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // The existing shaders don't use vertex input, so we'll disable vertex input
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
            {},
            0,
            nullptr,
            0,
            nullptr
        );

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
            {},
            vk::PrimitiveTopology::eTriangleList,
            VK_FALSE
        );

        vk::PipelineViewportStateCreateInfo viewportState(
            {},
            1,
            nullptr,
            1,
            nullptr
        );

        vk::PipelineRasterizationStateCreateInfo rasterizer(
            {},
            VK_FALSE,
            VK_FALSE,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eClockwise,
            VK_FALSE,
            0.0f,
            0.0f,
            0.0f,
            1.0f
        );

        vk::PipelineMultisampleStateCreateInfo multisampling(
            {},
            vk::SampleCountFlagBits::e1,
            VK_FALSE
        );

        vk::PipelineColorBlendAttachmentState colorBlendAttachment(
            VK_FALSE,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
        );

        vk::PipelineColorBlendStateCreateInfo colorBlending(
            {},
            VK_FALSE,
            vk::LogicOp::eCopy,
            1,
            &colorBlendAttachment
        );

        std::vector<vk::DynamicState> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo dynamicState(
            {},
            static_cast<uint32_t>(dynamicStates.size()),
            dynamicStates.data()
        );

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
            {},
            0,
            nullptr,
            0,
            nullptr
        );

        pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

        vk::GraphicsPipelineCreateInfo pipelineInfo(
            {},
            2,
            shaderStages,
            &vertexInputInfo,
            &inputAssembly,
            nullptr,
            &viewportState,
            &rasterizer,
            &multisampling,
            nullptr,
            &colorBlending,
            &dynamicState,
            pipelineLayout,
            renderPass,  // Use the render pass instead of nullptr
            0
        );

        auto result = device.createGraphicsPipeline(nullptr, pipelineInfo);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
        graphicsPipeline = result.value;

        device.destroyShaderModule(fragShaderModule);
        device.destroyShaderModule(vertShaderModule);
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            vk::ImageView attachments[] = {
                swapChainImageViews[i]
            };

            vk::FramebufferCreateInfo framebufferInfo(
                {},
                renderPass,
                1,
                attachments,
                swapChainExtent.width,
                swapChainExtent.height,
                1
            );

            swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
        }
    }

    void createCommandPool() {
        vk::CommandPoolCreateInfo poolInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            queueIndex
        );

        commandPool = device.createCommandPool(poolInfo);
    }

    void createVertexBuffer() {
        // The existing shaders don't use vertex buffers, so we'll skip this
    }

    void createIndexBuffer() {
        // The existing shaders don't use index buffers, so we'll skip this
    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        vk::CommandBufferAllocateInfo allocInfo(
            commandPool,
            vk::CommandBufferLevel::ePrimary,
            MAX_FRAMES_IN_FLIGHT
        );

        commandBuffers = device.allocateCommandBuffers(allocInfo);
    }

    void recordCommandBuffer(uint32_t imageIndex) {
        vk::CommandBufferBeginInfo beginInfo{};
        commandBuffers[currentFrame].begin(beginInfo);

        vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        vk::RenderPassBeginInfo renderPassInfo(
            renderPass,
            swapChainFramebuffers[imageIndex],
            { {0, 0}, swapChainExtent },
            1,
            &clearColor
        );

        commandBuffers[currentFrame].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

        vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f);
        commandBuffers[currentFrame].setViewport(0, 1, &viewport);

        vk::Rect2D scissor({0, 0}, swapChainExtent);
        commandBuffers[currentFrame].setScissor(0, 1, &scissor);

        // Draw triangle with only 3 vertices since the new shader uses a simple triangle
        commandBuffers[currentFrame].draw(3, 1, 0, 0);

        commandBuffers[currentFrame].endRenderPass();
        commandBuffers[currentFrame].end();
    }

    void createSyncObjects() {
        presentCompleteSemaphore.resize(swapChainImages.size());
        renderFinishedSemaphore.resize(swapChainImages.size());
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        vk::SemaphoreCreateInfo semaphoreInfo{};
        vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            presentCompleteSemaphore[i] = device.createSemaphore(semaphoreInfo);
            renderFinishedSemaphore[i] = device.createSemaphore(semaphoreInfo);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            inFlightFences[i] = device.createFence(fenceInfo);
        }
    }

    void drawFrame() {
        auto waitResult = device.waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        if (waitResult != vk::Result::eSuccess) {
            throw std::runtime_error("failed to wait for fence!");
        }

        auto result = device.acquireNextImageKHR(swapChain, UINT64_MAX, presentCompleteSemaphore[semaphoreIndex], nullptr);
        if (result.result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapChain();
            return;
        }
        if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        uint32_t imageIndex = result.value;

        auto resetResult = device.resetFences(1, &inFlightFences[currentFrame]);
        if (resetResult != vk::Result::eSuccess) {
            throw std::runtime_error("failed to reset fence!");
        }

        commandBuffers[currentFrame].reset();
        recordCommandBuffer(imageIndex);

        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::SubmitInfo submitInfo(
            1,
            &presentCompleteSemaphore[semaphoreIndex],
            waitStages,
            1,
            &commandBuffers[currentFrame],
            1,
            &renderFinishedSemaphore[imageIndex]
        );

        auto submitResult = queue.submit(1, &submitInfo, inFlightFences[currentFrame]);
        if (submitResult != vk::Result::eSuccess) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        vk::PresentInfoKHR presentInfo(
            1,
            &renderFinishedSemaphore[imageIndex],
            1,
            &swapChain,
            &imageIndex
        );

        auto presentResult = queue.presentKHR(presentInfo);
        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        } else if (presentResult != vk::Result::eSuccess) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        semaphoreIndex = (semaphoreIndex + 1) % presentCompleteSemaphore.size();
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    vk::ShaderModule createShaderModule(const std::vector<char>& code) {
        vk::ShaderModuleCreateInfo createInfo(
            {},
            code.size(),
            reinterpret_cast<const uint32_t*>(code.data())
        );

        return device.createShaderModule(createInfo);
    }

    static vk::Format chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
        for (const auto& format : availableFormats) {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return format.format;
            }
        }
        return availableFormats[0].format;
    }

    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            }
        }
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        vk::Extent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
};

int main() {
    try {
        HelloTriangleApplication app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
