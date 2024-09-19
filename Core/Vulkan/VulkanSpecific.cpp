#include "pch.h"
#include "VulkanSpecific.h"
#include "volk.h"
namespace RHI
{

    void log(LogLevel lvl, std::string_view msg)
    {
        if(vInstance::current->logCallback)
        {
            vInstance::current->logCallback(lvl, msg);
        }
    }
    VkFormat FormatConv(const RHI::Format format)
    {
        return static_cast<VkFormat>(FormatToVk(format));
    }
    CreationError marshall_error(VkResult r)
    {
        switch(r)
        {
            case(VK_ERROR_OUT_OF_DEVICE_MEMORY): return CreationError::OutOfDeviceMemory;
            case(VK_ERROR_OUT_OF_HOST_MEMORY): return CreationError::OutOfHostMemory;
            case(VK_SUCCESS): return CreationError::None;
            case(VK_ERROR_FRAGMENTED_POOL): return CreationError::FragmentedHeap;
            case(VK_ERROR_OUT_OF_POOL_MEMORY): return CreationError::OutOfHeapMemory;
            default: return CreationError::Unknown;
        }
    }
    VkPrimitiveTopology vkPrimitiveTopology(PrimitiveTopology topology)
    {
        switch (topology)
        {
        case RHI::PrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case RHI::PrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case RHI::PrimitiveTopology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case RHI::PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case RHI::PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        default: return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        }
    }
    std::pair<QueueFamilyIndices,std::vector<uint32_t>> findQueueFamilyIndices(RHI::PhysicalDevice* device, RHI::Surface surface)
    {
        QueueFamilyIndices indices = {};
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties((VkPhysicalDevice)device->ID, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties((VkPhysicalDevice)device->ID, &queueFamilyCount, queueFamilies.data());
        std::vector<uint32_t> count(queueFamilyCount);
        for (uint32_t i = 0; i < queueFamilyCount; i++)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsIndex = i;
                count[i] = queueFamilies[i].queueCount;
                indices.flags |= HasGraphics;
            }
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                indices.computeIndex = i;
                count[i] = queueFamilies[i].queueCount;
                indices.flags |= HasCompute;
            }
            if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                indices.copyIndex = i;
                count[i] = queueFamilies[i].queueCount;
                indices.flags |= HasCopy;
            }
            if (surface.ID)
            {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR((VkPhysicalDevice)device->ID, i, (VkSurfaceKHR)surface.ID, &presentSupport);

                if (presentSupport && !(indices.flags & HasPresent)) {
                    indices.presentIndex = i;
                    indices.flags |= HasPresent;
                }
            }
        }
        return { indices,count };
    }
}