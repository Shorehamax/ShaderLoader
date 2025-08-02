//
// Created by charlie on 8/1/25.
//

#ifndef IVULKANCONTEXT_H
#define IVULKANCONTEXT_H

#include <vulkan/vulkan.h>
#include <string>
#include <memory>
namespace VulkanCore {

    class IVulkanContext {
    public:
        virtual ~IVulkanContext() = default;
        virtual bool     initialize(const std::string& applicationName) = 0;
        virtual bool     initializeWithExistingInstance(VkInstance existingInstance) = 0;
        virtual void     cleanup() = 0;
        virtual VkInstance       instance() const = 0;
        virtual VkDevice         device() const = 0;
        virtual VkPhysicalDevice physicalDevice() const = 0;
        virtual VkQueue          graphicsQueue() const = 0;
        virtual uint32_t         graphicsQueueFamilyIndex() const = 0;
    };

    // Factory to create the default VulkanContext
    std::shared_ptr<IVulkanContext> createVulkanContext();

} // namespace VulkanCore


#endif //IVULKANCONTEXT_H
