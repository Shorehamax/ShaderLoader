//
// Created by charlie on 8/2/25.
//

#include "../Public/QtVulkanWindow.h"
#include "../Public/IVulkanContext.h"
#include "../Public/VulkanRenderer.h"
#include "../../ShaderLoader/Public/ShaderLoader.h"
#include "../../ShaderLoader/Public/IShaderCompiler.h"
#include <vulkan/vulkan.h>
#include <QVulkanDeviceFunctions>

namespace VulkanCore {

    class VulkanWindowRenderer : public QVulkanWindowRenderer {
    public:
        VulkanWindowRenderer(QVulkanWindow* w)
            : m_window(w)
        {
            m_context = createVulkanContext();
            // Use Qt's existing Vulkan instance instead of creating our own
            VkInstance qtInstance = w->vulkanInstance()->vkInstance();
            m_context->initializeWithExistingInstance(qtInstance);
        }

        void initResources() override {
            // No long‐lived resources here; Qt handles instance & surface
        }

        void initSwapChainResources() override {
            m_renderer = std::make_unique<VulkanRenderer>(m_context);
            m_renderer->initialize(m_window);

            // Create default pipeline if shaders are available
            if (m_vertexSpirv.empty() || m_fragmentSpirv.empty()) {
                createDefaultShaders();
            }
            createGraphicsPipeline();
        }

        void releaseSwapChainResources() override {
            if (m_renderer) {
                m_renderer->cleanup();
                m_renderer.reset();
            }
        }

        void releaseResources() override {
            if (m_context) {
                m_context->cleanup();
                m_context.reset();
            }
        }

        void startNextFrame() override {
            if (!m_renderer || m_renderer->getPipeline() == VK_NULL_HANDLE) {
                m_window->frameReady();
                return;
            }

            m_renderer->drawFrame([this](VkCommandBuffer cmd) {
                // Get Qt's Vulkan functions
                QVulkanDeviceFunctions *df = m_window->vulkanInstance()->deviceFunctions(m_context->device());

                // Begin render pass
                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = m_window->defaultRenderPass();
                renderPassInfo.framebuffer = m_window->currentFramebuffer();
                renderPassInfo.renderArea.offset = {0, 0};
                renderPassInfo.renderArea.extent = {
                    static_cast<uint32_t>(m_window->swapChainImageSize().width()),
                    static_cast<uint32_t>(m_window->swapChainImageSize().height())
                };

                VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
                renderPassInfo.clearValueCount = 1;
                renderPassInfo.pClearValues = &clearColor;

                df->vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                // Bind pipeline and draw
                df->vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_renderer->getPipeline());
                df->vkCmdDraw(cmd, 3, 1, 0, 0); // Draw triangle with 3 vertices

                df->vkCmdEndRenderPass(cmd);
            });

            m_window->frameReady();
        }

        // Method to load custom shaders
        bool loadShaders(const std::vector<uint32_t>& vertexSpirv, const std::vector<uint32_t>& fragmentSpirv) {
            m_vertexSpirv = vertexSpirv;
            m_fragmentSpirv = fragmentSpirv;

            if (m_renderer) {
                return createGraphicsPipeline();
            }
            return true; // Will be created when renderer is initialized
        }

    private:
        QVulkanWindow* m_window;
        std::shared_ptr<IVulkanContext> m_context;
        std::unique_ptr<VulkanRenderer> m_renderer;
        std::vector<uint32_t> m_vertexSpirv;
        std::vector<uint32_t> m_fragmentSpirv;

        void createDefaultShaders() {
            // Default vertex shader (hardcoded SPIR-V for a simple triangle)
            m_vertexSpirv = {
                0x07230203, 0x00010000, 0x00080007, 0x0000002e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
                0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
                0x0008000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000b, 0x0000001b, 0x0000001c,
                0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00060005,
                0x00000009, 0x69736f70, 0x6e6f6974, 0x00000073, 0x00000000, 0x00050005, 0x0000000b, 0x56206c67,
                0x65747265, 0x6e490078, 0x00786564, 0x00060005, 0x00000011, 0x505f6c67, 0x65567265, 0x78657472,
                0x00000000, 0x00060006, 0x00000011, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00070006,
                0x00000011, 0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953, 0x00000000, 0x00070006, 0x00000011,
                0x00000002, 0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e, 0x00070006, 0x00000011, 0x00000003,
                0x435f6c67, 0x446c6c75, 0x61747369, 0x0065636e, 0x00030005, 0x00000013, 0x00006c67, 0x00050005,
                0x0000001b, 0x67617266, 0x6f6c6f43, 0x00000072, 0x00050005, 0x0000001c, 0x6f6c6f63, 0x00007372,
                0x00000000, 0x00040047, 0x0000000b, 0x0000000b, 0x0000002a, 0x00050048, 0x00000011, 0x00000000,
                0x0000000b, 0x00000000, 0x00050048, 0x00000011, 0x00000001, 0x0000000b, 0x00000001, 0x00050048,
                0x00000011, 0x00000002, 0x0000000b, 0x00000003, 0x00050048, 0x00000011, 0x00000003, 0x0000000b,
                0x00000004, 0x00030047, 0x00000011, 0x00000002, 0x00040047, 0x0000001b, 0x0000001e, 0x00000000,
                0x00040047, 0x0000001c, 0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003,
                0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000002,
                0x00040015, 0x00000008, 0x00000020, 0x00000000, 0x0004002b, 0x00000008, 0x00000009, 0x00000003,
                0x0004001c, 0x0000000a, 0x00000007, 0x00000009, 0x00040020, 0x0000000b, 0x00000001, 0x00000008,
                0x0004003b, 0x0000000b, 0x0000000c, 0x00000001, 0x00040017, 0x0000000d, 0x00000006, 0x00000004,
                0x00040015, 0x0000000e, 0x00000020, 0x00000001, 0x0004002b, 0x0000000e, 0x0000000f, 0x00000001,
                0x0004001c, 0x00000010, 0x00000006, 0x0000000f, 0x0006001e, 0x00000011, 0x0000000d, 0x00000006,
                0x00000010, 0x00000010, 0x00040020, 0x00000012, 0x00000003, 0x00000011, 0x0004003b, 0x00000012,
                0x00000013, 0x00000003, 0x00040015, 0x00000014, 0x00000020, 0x00000001, 0x0004002b, 0x00000014,
                0x00000015, 0x00000000, 0x00040020, 0x00000016, 0x00000006, 0x00000007, 0x0004002b, 0x00000006,
                0x00000018, 0x00000000, 0x0004002b, 0x00000006, 0x00000019, 0xbf000000, 0x0007002c, 0x00000007,
                0x0000001a, 0x00000018, 0x00000019, 0x0004002b, 0x00000006, 0x0000001b, 0x3f000000, 0x0007002c,
                0x00000007, 0x0000001c, 0x0000001b, 0x0000001b, 0x0007002c, 0x00000007, 0x0000001d, 0xbf000000,
                0x0000001b, 0x0007002c, 0x0000000a, 0x0000001e, 0x0000001a, 0x0000001c, 0x0000001d, 0x00040020,
                0x0000001f, 0x00000006, 0x0000000a, 0x00040017, 0x00000020, 0x00000006, 0x00000003, 0x0004002b,
                0x00000006, 0x00000021, 0x3f800000, 0x0007002c, 0x00000020, 0x00000022, 0x0000001b, 0x00000018,
                0x00000018, 0x0007002c, 0x00000020, 0x00000023, 0x00000018, 0x0000001b, 0x00000018, 0x0007002c,
                0x00000020, 0x00000024, 0x00000018, 0x00000018, 0x0000001b, 0x0004002b, 0x00000008, 0x00000025,
                0x00000003, 0x0004001c, 0x00000026, 0x00000020, 0x00000025, 0x0007002c, 0x00000026, 0x00000027,
                0x00000022, 0x00000023, 0x00000024, 0x00040020, 0x00000028, 0x00000006, 0x00000026, 0x00040020,
                0x00000029, 0x00000003, 0x00000020, 0x0004003b, 0x00000029, 0x0000002a, 0x00000003, 0x00040020,
                0x0000002b, 0x00000020, 0x00000020, 0x00040020, 0x0000002c, 0x00000003, 0x0000000d, 0x0004002b,
                0x00000006, 0x0000002d, 0x3f800000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003,
                0x000200f8, 0x00000005, 0x0004003d, 0x00000008, 0x0000000c, 0x0000000b, 0x00040041, 0x00000016,
                0x00000017, 0x0000000009, 0x0000000c, 0x0004003d, 0x00000007, 0x00000007, 0x00000017, 0x00050051,
                0x00000006, 0x0000000a, 0x00000007, 0x00000000, 0x00050051, 0x00000006, 0x0000000b, 0x00000007,
                0x00000001, 0x00070050, 0x0000000d, 0x0000000c, 0x0000000a, 0x0000000b, 0x00000018, 0x0000002d,
                0x00040041, 0x0000002c, 0x0000000d, 0x00000013, 0x00000015, 0x0003003e, 0x0000000d, 0x0000000c,
                0x0004003d, 0x00000008, 0x0000000e, 0x0000000b, 0x00040041, 0x0000002b, 0x0000000f, 0x00000027,
                0x0000000e, 0x0004003d, 0x00000020, 0x00000010, 0x0000000f, 0x0003003e, 0x0000002a, 0x00000010,
                0x000100fd, 0x00010038
            };

            // Default fragment shader (hardcoded SPIR-V for colored output)
            m_fragmentSpirv = {
                0x07230203, 0x00010000, 0x00080007, 0x0000001e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
                0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
                0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000d, 0x00030010,
                0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
                0x00000000, 0x00040005, 0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000, 0x00040005, 0x0000000d,
                0x67617266, 0x6f6c6f43, 0x00000072, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047,
                0x0000000d, 0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002,
                0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020,
                0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040017,
                0x0000000a, 0x00000006, 0x00000003, 0x00040020, 0x0000000b, 0x00000001, 0x0000000a, 0x0004003b,
                0x0000000b, 0x0000000d, 0x00000001, 0x0004002b, 0x00000006, 0x0000000f, 0x3f800000, 0x00050036,
                0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x0000000a,
                0x0000000e, 0x0000000d, 0x00050051, 0x00000006, 0x00000010, 0x0000000e, 0x00000000, 0x00050051,
                0x00000006, 0x00000011, 0x0000000e, 0x00000001, 0x00050051, 0x00000006, 0x00000012, 0x0000000e,
                0x00000002, 0x00070050, 0x00000007, 0x00000013, 0x00000010, 0x00000011, 0x00000012, 0x0000000f,
                0x0003003e, 0x00000009, 0x00000013, 0x000100fd, 0x00010038
            };
        }

        bool createGraphicsPipeline() {
            if (!m_renderer || m_vertexSpirv.empty() || m_fragmentSpirv.empty()) {
                return false;
            }

            PipelineCreateInfo createInfo;
            createInfo.vertexSpirv = m_vertexSpirv;
            createInfo.fragmentSpirv = m_fragmentSpirv;
            createInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            return m_renderer->createPipeline(createInfo);
        }
    };

    QVulkanWindowRenderer* QtVulkanWindow::createRenderer() {
        m_renderer = new VulkanWindowRenderer(this);
        return m_renderer;
    }

    bool QtVulkanWindow::loadShadersFromFiles(const QString& vertexShaderPath, const QString& fragmentShaderPath) {
        // Use the existing ShaderLoader infrastructure
        auto compiler = ShaderLoader::createDefaultCompiler();
        ShaderLoader::ShaderLoader loader(std::move(compiler));

        // Load vertex shader
        if (!loader.loadShader(vertexShaderPath.toStdString())) {
            return false;
        }
        auto* vertexModule = loader.getModule(vertexShaderPath.toStdString());
        if (!vertexModule || vertexModule->spirv.empty()) {
            return false;
        }

        // Load fragment shader
        if (!loader.loadShader(fragmentShaderPath.toStdString())) {
            return false;
        }
        auto* fragmentModule = loader.getModule(fragmentShaderPath.toStdString());
        if (!fragmentModule || fragmentModule->spirv.empty()) {
            return false;
        }

        // Load shaders into the renderer
        if (m_renderer) {
            auto* vulkanRenderer = static_cast<VulkanWindowRenderer*>(m_renderer);
            return vulkanRenderer->loadShaders(vertexModule->spirv, fragmentModule->spirv);
        }

        return true; // Will be loaded when renderer is created
    }

} // namespace VulkanCore
