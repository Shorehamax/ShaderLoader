//
// Created by charlie on 8/1/25.
//

#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include "IVulkanContext.h"
#include <functional>
#include <vector>
#include <vulkan/vulkan.h>
#include <QVulkanWindow>

namespace VulkanCore {

    struct PipelineCreateInfo {
        std::vector<uint32_t> vertexSpirv;
        std::vector<uint32_t> fragmentSpirv;
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    };

    class VulkanRenderer {
    public:
        explicit VulkanRenderer(std::shared_ptr<IVulkanContext> context);
        ~VulkanRenderer();

        bool initialize(void* nativeWindowHandle);
        void cleanup();

        // Pipeline management
        bool createPipeline(const PipelineCreateInfo& createInfo);
        void destroyPipeline();

        void drawFrame(const std::function<void(VkCommandBuffer)>& recordCommands);
        void waitIdle();

        // Getters for pipeline components
        VkPipeline getPipeline() const { return m_pipeline; }
        VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }

    private:
        std::shared_ptr<IVulkanContext> m_context;
        QVulkanWindow*                  m_window{nullptr};

        VkSurfaceKHR                    m_surface{VK_NULL_HANDLE};
        VkSwapchainKHR                  m_swapchain{VK_NULL_HANDLE};
        std::vector<VkImage>            m_swapchainImages;
        VkExtent2D                      m_swapchainExtent{};
        VkFormat                        m_swapchainImageFormat{VK_FORMAT_UNDEFINED};
        std::vector<VkImageView>        m_imageViews;
        VkRenderPass                    m_renderPass{VK_NULL_HANDLE};
        std::vector<VkFramebuffer>      m_framebuffers;
        VkCommandPool                   m_commandPool{VK_NULL_HANDLE};
        std::vector<VkCommandBuffer>    m_commandBuffers;

        // Pipeline components
        VkPipelineLayout                m_pipelineLayout{VK_NULL_HANDLE};
        VkPipeline                      m_pipeline{VK_NULL_HANDLE};

        // Vertex buffer for a simple triangle
        VkBuffer                        m_vertexBuffer{VK_NULL_HANDLE};
        VkDeviceMemory                  m_vertexBufferMemory{VK_NULL_HANDLE};

        // Synchronization objects
        static const int                MAX_FRAMES_IN_FLIGHT = 2;
        std::vector<VkSemaphore>        m_imageAvailableSemaphores;
        std::vector<VkSemaphore>        m_renderFinishedSemaphores;
        std::vector<VkFence>            m_inFlightFences;
        uint32_t                        m_currentFrame{0};

        bool createSurface();
        bool createSwapchain();
        bool createImageViews();
        bool createRenderPass();
        bool createFramebuffers();
        bool createCommandPool();
        bool allocateCommandBuffers();
        bool createVertexBuffer();
        bool createSyncObjects();
        void cleanupSwapchain();

        VkShaderModule createShaderModule(const std::vector<uint32_t>& code);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    };

} // namespace VulkanCore


#endif //VULKANRENDERER_H
