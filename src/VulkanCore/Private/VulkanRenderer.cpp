//
// Created by charlie on 8/1/25.
//

// src/VulkanCore/Private/VulkanRenderer.cpp

// src/VulkanCore/Private/VulkanRenderer.cpp

#include "../Public/VulkanRenderer.h"
#include <QVulkanWindow>
#include <QVulkanInstance>
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace VulkanCore {

// Helpers for swapchain setup
static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (auto& fmt : formats) {
        if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
            fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return fmt;
    }
    return formats[0];
}

static VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (auto& mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps, QVulkanWindow* window) {
    if (caps.currentExtent.width != UINT32_MAX)
        return caps.currentExtent;
    // fall back to window size
    return {
        static_cast<uint32_t>(window->width()),
        static_cast<uint32_t>(window->height())
    };
}

VulkanRenderer::VulkanRenderer(std::shared_ptr<IVulkanContext> context)
    : m_context(std::move(context))
{}

VulkanRenderer::~VulkanRenderer() {
    cleanup();
}

bool VulkanRenderer::initialize(void* nativeWindowHandle) {
    m_window = static_cast<QVulkanWindow*>(nativeWindowHandle);

    if (!createSurface())        return false;
    if (!createSwapchain())      return false;
    if (!createImageViews())     return false;
    if (!createRenderPass())     return false;
    if (!createFramebuffers())   return false;
    if (!createCommandPool())    return false;
    if (!allocateCommandBuffers()) return false;
    if (!createVertexBuffer())   return false;
    if (!createSyncObjects())    return false;

    return true;
}

void VulkanRenderer::cleanup() {
    waitIdle();

    // Cleanup synchronization objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (i < m_imageAvailableSemaphores.size()) {
            vkDestroySemaphore(m_context->device(), m_imageAvailableSemaphores[i], nullptr);
        }
        if (i < m_renderFinishedSemaphores.size()) {
            vkDestroySemaphore(m_context->device(), m_renderFinishedSemaphores[i], nullptr);
        }
        if (i < m_inFlightFences.size()) {
            vkDestroyFence(m_context->device(), m_inFlightFences[i], nullptr);
        }
    }

    // Cleanup pipeline
    destroyPipeline();

    // Cleanup vertex buffer
    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_context->device(), m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }
    if (m_vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_context->device(), m_vertexBufferMemory, nullptr);
        m_vertexBufferMemory = VK_NULL_HANDLE;
    }

    cleanupSwapchain();

    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_context->instance(), m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_context->device(), m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::drawFrame(const std::function<void(VkCommandBuffer)>& recordCommands) {
    // When using Qt's QVulkanWindow, we need to work within Qt's rendering framework
    // Qt will call our renderer through the QVulkanWindowRenderer interface
    // For now, we'll use a simplified approach that works with Qt's command buffer

    // Get the device from our context for basic operations
    VkDevice device = m_context->device();

    // For Qt integration, the actual command buffer recording should happen
    // in the QVulkanWindowRenderer's startNextFrame method
    // This method serves as a bridge between the two systems

    // We can still call the recordCommands function, but we need a valid command buffer
    // In a proper Qt integration, this would be provided by Qt's renderer
    if (!m_commandBuffers.empty()) {
        VkCommandBuffer cmd = m_commandBuffers[0]; // Use first available command buffer

        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &bi);

        recordCommands(cmd);

        vkEndCommandBuffer(cmd);

        // Submit the command buffer
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cmd;

        vkQueueSubmit(m_context->graphicsQueue(), 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_context->graphicsQueue());
    }
}

void VulkanRenderer::waitIdle() {
    vkDeviceWaitIdle(m_context->device());
}

bool VulkanRenderer::createSurface() {
    // Qt manages the surface internally, we can get it through the Vulkan instance
    m_surface = QVulkanInstance::surfaceForWindow(m_window);
    return m_surface != VK_NULL_HANDLE;
}

bool VulkanRenderer::createSwapchain() {
    VkPhysicalDevice phys = m_context->physicalDevice();
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, m_surface, &caps);

    uint32_t fc=0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, m_surface, &fc, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(fc);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, m_surface, &fc, fmts.data());

    uint32_t pm=0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, m_surface, &pm, nullptr);
    std::vector<VkPresentModeKHR> pms(pm);
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, m_surface, &pm, pms.data());

    auto sf = chooseSwapSurfaceFormat(fmts);
    auto pmode = choosePresentMode(pms);
    m_swapchainExtent = chooseExtent(caps, m_window);

    // Store the swapchain format for consistent use
    m_swapchainImageFormat = sf.format;

    uint32_t ic = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && ic > caps.maxImageCount)
        ic = caps.maxImageCount;

    VkSwapchainCreateInfoKHR sci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    sci.surface            = m_surface;
    sci.minImageCount      = ic;
    sci.imageFormat        = sf.format;
    sci.imageColorSpace    = sf.colorSpace;
    sci.imageExtent        = m_swapchainExtent;
    sci.imageArrayLayers   = 1;
    sci.imageUsage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.preTransform       = caps.currentTransform;
    sci.compositeAlpha     = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode        = pmode;
    sci.clipped            = VK_TRUE;
    sci.oldSwapchain       = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_context->device(), &sci, nullptr, &m_swapchain) != VK_SUCCESS)
        return false;

    uint32_t cnt;
    vkGetSwapchainImagesKHR(m_context->device(), m_swapchain, &cnt, nullptr);
    m_swapchainImages.resize(cnt);
    vkGetSwapchainImagesKHR(m_context->device(), m_swapchain, &cnt, m_swapchainImages.data());
    return true;
}

bool VulkanRenderer::createImageViews() {
    m_imageViews.resize(m_swapchainImages.size());
    for (size_t i=0; i<m_swapchainImages.size(); ++i) {
        VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        iv.image            = m_swapchainImages[i];
        iv.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        iv.format           = m_swapchainImageFormat; // Use actual swapchain format
        iv.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0,1, 0,1};
        if (vkCreateImageView(m_context->device(), &iv, nullptr, &m_imageViews[i]) != VK_SUCCESS)
            return false;
    }
    return true;
}

bool VulkanRenderer::createRenderPass() {
    VkAttachmentDescription ad{};
    ad.format        = m_swapchainImageFormat; // Use actual swapchain format
    ad.samples       = VK_SAMPLE_COUNT_1_BIT;
    ad.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ad.storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
    ad.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ad.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference ar{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription sp{};
    sp.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sp.colorAttachmentCount = 1;
    sp.pColorAttachments    = &ar;

    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rp.attachmentCount = 1;
    rp.pAttachments    = &ad;
    rp.subpassCount    = 1;
    rp.pSubpasses      = &sp;

    return vkCreateRenderPass(m_context->device(), &rp, nullptr, &m_renderPass) == VK_SUCCESS;
}

bool VulkanRenderer::createFramebuffers() {
    m_framebuffers.resize(m_imageViews.size());
    for (size_t i=0; i<m_imageViews.size(); ++i) {
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass      = m_renderPass;
        fb.attachmentCount = 1;
        fb.pAttachments    = &m_imageViews[i];
        fb.width           = m_swapchainExtent.width;
        fb.height          = m_swapchainExtent.height;
        fb.layers          = 1;
        if (vkCreateFramebuffer(m_context->device(), &fb, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
            return false;
    }
    return true;
}

bool VulkanRenderer::createCommandPool() {
    VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    cp.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cp.queueFamilyIndex = m_context->graphicsQueueFamilyIndex();
    return vkCreateCommandPool(m_context->device(), &cp, nullptr, &m_commandPool) == VK_SUCCESS;
}

bool VulkanRenderer::allocateCommandBuffers() {
    m_commandBuffers.resize(m_framebuffers.size());
    VkCommandBufferAllocateInfo abi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    abi.commandPool        = m_commandPool;
    abi.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    abi.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
    return vkAllocateCommandBuffers(m_context->device(), &abi, m_commandBuffers.data()) == VK_SUCCESS;
}

void VulkanRenderer::cleanupSwapchain() {
    for (auto& fb : m_framebuffers) vkDestroyFramebuffer(m_context->device(), fb, nullptr);
    m_framebuffers.clear();
    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_context->device(), m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }
    for (auto& iv : m_imageViews) vkDestroyImageView(m_context->device(), iv, nullptr);
    m_imageViews.clear();
    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_context->device(), m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

bool VulkanRenderer::createPipeline(const PipelineCreateInfo& createInfo) {
    // Destroy existing pipeline if it exists
    destroyPipeline();

    // Create shader modules
    VkShaderModule vertexShaderModule = createShaderModule(createInfo.vertexSpirv);
    VkShaderModule fragmentShaderModule = createShaderModule(createInfo.fragmentSpirv);

    if (vertexShaderModule == VK_NULL_HANDLE || fragmentShaderModule == VK_NULL_HANDLE) {
        if (vertexShaderModule != VK_NULL_HANDLE) vkDestroyShaderModule(m_context->device(), vertexShaderModule, nullptr);
        if (fragmentShaderModule != VK_NULL_HANDLE) vkDestroyShaderModule(m_context->device(), fragmentShaderModule, nullptr);
        std::cerr << "Failed to create shader modules." << std::endl;
        return false;
    }

    // Shader stage creation
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertexShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragmentShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Vertex input (empty for simple triangle with hardcoded vertices)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = createInfo.topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swapchainExtent.width;
    viewport.height = (float)m_swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(m_context->device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(m_context->device(), vertexShaderModule, nullptr);
        vkDestroyShaderModule(m_context->device(), fragmentShaderModule, nullptr);
        std::cerr << "Failed to create pipeline layout." << std::endl;
        return false;
    }

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;

    // Add detailed error checking
    VkResult result = vkCreateGraphicsPipelines(m_context->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create graphics pipeline. VkResult: " << result << std::endl;

        // Print detailed error information
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                std::cerr << "Error: VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                std::cerr << "Error: VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
                break;
            case VK_ERROR_INVALID_SHADER_NV:
                std::cerr << "Error: VK_ERROR_INVALID_SHADER_NV" << std::endl;
                break;
            default:
                std::cerr << "Error: Unknown Vulkan error code: " << result << std::endl;
                break;
        }

        // Print debug information about pipeline components
        std::cerr << "Debug info:" << std::endl;
        std::cerr << "  Device: " << (m_context->device() ? "Valid" : "NULL") << std::endl;
        std::cerr << "  Render pass: " << (m_renderPass != VK_NULL_HANDLE ? "Valid" : "NULL") << std::endl;
        std::cerr << "  Pipeline layout: " << (m_pipelineLayout != VK_NULL_HANDLE ? "Valid" : "NULL") << std::endl;
        std::cerr << "  Vertex shader module: " << (vertexShaderModule != VK_NULL_HANDLE ? "Valid" : "NULL") << std::endl;
        std::cerr << "  Fragment shader module: " << (fragmentShaderModule != VK_NULL_HANDLE ? "Valid" : "NULL") << std::endl;
        std::cerr << "  Swapchain format: " << m_swapchainImageFormat << std::endl;
        std::cerr << "  Swapchain extent: " << m_swapchainExtent.width << "x" << m_swapchainExtent.height << std::endl;

        vkDestroyShaderModule(m_context->device(), vertexShaderModule, nullptr);
        vkDestroyShaderModule(m_context->device(), fragmentShaderModule, nullptr);
        return false;
    }

    // Clean up shader modules (they're no longer needed after pipeline creation)
    vkDestroyShaderModule(m_context->device(), vertexShaderModule, nullptr);
    vkDestroyShaderModule(m_context->device(), fragmentShaderModule, nullptr);

    return true;
}

void VulkanRenderer::destroyPipeline() {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_context->device(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_context->device(), m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
}

bool VulkanRenderer::createVertexBuffer() {
    // Simple triangle vertices (position only, colors in shader)
    const float vertices[] = {
        0.0f, -0.5f,  // Top vertex
        0.5f,  0.5f,  // Bottom right
        -0.5f, 0.5f   // Bottom left
    };

    VkDeviceSize bufferSize = sizeof(vertices);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_context->device(), &bufferInfo, nullptr, &m_vertexBuffer) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_context->device(), m_vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_context->device(), &allocInfo, nullptr, &m_vertexBufferMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindBufferMemory(m_context->device(), m_vertexBuffer, m_vertexBufferMemory, 0);

    // Copy vertex data
    void* data;
    vkMapMemory(m_context->device(), m_vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, (size_t) bufferSize);
    vkUnmapMemory(m_context->device(), m_vertexBufferMemory);

    return true;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_context->device(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cerr << "Failed to create shader module. Code size: " << createInfo.codeSize << " bytes." << std::endl;
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_context->physicalDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

bool VulkanRenderer::createSyncObjects() {
    VkDevice device = m_context->device();

    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create synchronization objects for frame " << i << std::endl;
            return false;
        }
    }

    return true;
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    // Begin command buffer recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind graphics pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    // Set viewport and scissor dynamically (good practice)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchainExtent.width);
    viewport.height = static_cast<float>(m_swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Draw the triangle (3 vertices, no vertex buffer needed as vertices are hardcoded in shader)
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // End render pass
    vkCmdEndRenderPass(commandBuffer);

    // End command buffer recording
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

} // namespace VulkanCore
