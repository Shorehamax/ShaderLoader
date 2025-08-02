//
// Created by charlie on 8/1/25.
//

// src/VulkanCore/Private/VulkanContext.cpp

// src/VulkanCore/Private/VulkanContext.cpp

#include "../Public/IVulkanContext.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include <memory>

namespace VulkanCore {

class VulkanContext : public IVulkanContext {
public:
    VulkanContext() = default;
    ~VulkanContext() override { cleanup(); }

    bool initialize(const std::string& applicationName) override {
        if (!createInstance(applicationName))      return false;
        if (!pickPhysicalDevice())                  return false;
        if (!createLogicalDevice())                 return false;
        vkGetDeviceQueue(m_device, m_queueFamilyIndex, 0, &m_graphicsQueue);
        return true;
    }

    bool initializeWithExistingInstance(VkInstance existingInstance) override {
        m_instance = existingInstance;
        m_ownsInstance = false; // We don't own this instance, so don't destroy it
        if (!pickPhysicalDevice())                  return false;
        if (!createLogicalDevice())                 return false;
        vkGetDeviceQueue(m_device, m_queueFamilyIndex, 0, &m_graphicsQueue);
        return true;
    }

    void cleanup() override {
        if (m_device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(m_device);
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;
        }
        if (m_ownsInstance && m_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }
    }

    VkInstance       instance()       const override { return m_instance; }
    VkDevice         device()         const override { return m_device; }
    VkPhysicalDevice physicalDevice() const override { return m_physicalDevice; }
    VkQueue          graphicsQueue()  const override { return m_graphicsQueue; }
    uint32_t         graphicsQueueFamilyIndex() const override { return m_queueFamilyIndex; }

private:
    VkInstance       m_instance{VK_NULL_HANDLE};
    VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
    VkDevice         m_device{VK_NULL_HANDLE};
    VkQueue          m_graphicsQueue{VK_NULL_HANDLE};
    uint32_t         m_queueFamilyIndex{0};
    bool             m_ownsInstance{true}; // Assume we own the instance by default

    bool createInstance(const std::string& appName) {
        VkApplicationInfo appInfo{};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = appName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName        = "ShaderPlaygroundEngine";
        appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion         = VK_API_VERSION_1_3;

        // No platform extensions—Qt will provide surface support itself
        VkInstanceCreateInfo instCI{};
        instCI.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instCI.pApplicationInfo        = &appInfo;
        instCI.enabledExtensionCount   = 0;
        instCI.ppEnabledExtensionNames = nullptr;
        instCI.enabledLayerCount       = 0;
        instCI.ppEnabledLayerNames     = nullptr;

        return vkCreateInstance(&instCI, nullptr, &m_instance) == VK_SUCCESS;
    }

    bool pickPhysicalDevice() {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
        if (count == 0) return false;

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

        for (auto dev : devices) {
            uint32_t queueCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueCount, nullptr);
            std::vector<VkQueueFamilyProperties> families(queueCount);
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueCount, families.data());

            for (uint32_t i = 0; i < queueCount; ++i) {
                if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    m_physicalDevice    = dev;
                    m_queueFamilyIndex  = i;
                    return true;
                }
            }
        }
        return false;
    }

    bool createLogicalDevice() {
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCI{};
        queueCI.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCI.queueFamilyIndex = m_queueFamilyIndex;
        queueCI.queueCount       = 1;
        queueCI.pQueuePriorities = &queuePriority;

        // We still need the swapchain extension for rendering
        const char* deviceExts[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkPhysicalDeviceFeatures deviceFeatures{}; // enable as needed

        VkDeviceCreateInfo devCI{};
        devCI.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        devCI.queueCreateInfoCount    = 1;
        devCI.pQueueCreateInfos       = &queueCI;
        devCI.enabledExtensionCount   = 1;
        devCI.ppEnabledExtensionNames = deviceExts;
        devCI.pEnabledFeatures        = &deviceFeatures;

        return vkCreateDevice(m_physicalDevice, &devCI, nullptr, &m_device) == VK_SUCCESS;
    }
};

std::shared_ptr<IVulkanContext> createVulkanContext() {
    return std::make_shared<VulkanContext>();
}

} // namespace VulkanCore
