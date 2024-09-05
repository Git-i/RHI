#include "pch.h"
#include "VulkanSpecific.h"
#include "volk.h"
namespace RHI
{
    
    vInstance* vInstance::current = nullptr;
    void log(LogLevel lvl, std::string_view msg)
    {
        if(vInstance::current->logCallback)
        {
            vInstance::current->logCallback(lvl, msg);
        }
    }
    VkFormat FormatConv(RHI::Format format)
    {
        using enum Format;
        switch (format)
        {
            case(UNKNOWN): return VK_FORMAT_UNDEFINED;
            case(B4G4R4A4_UNORM): return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
            case(B5G6R5_UNORM): return VK_FORMAT_B5G6R5_UNORM_PACK16;
            case(B5G5R5A1_UNORM): return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
            case(R8_UNORM): return VK_FORMAT_R8_UNORM;
            case(R8_SNORM): return VK_FORMAT_R8_SNORM;
            case(R8_UINT): return VK_FORMAT_R8_UINT;
            case(R8_SINT): return VK_FORMAT_R8_SINT;

            case(R8G8_UNORM): return VK_FORMAT_R8G8_UNORM;
            case(R8G8_SNORM): return VK_FORMAT_R8G8_SNORM;
            case(R8G8_UINT): return VK_FORMAT_R8G8_UINT;
            case(R8G8_SINT): return VK_FORMAT_R8G8_SINT;

            case(R8G8B8A8_UNORM): return VK_FORMAT_R8G8B8A8_UNORM;
            case(R8G8B8A8_SNORM): return VK_FORMAT_R8G8B8A8_SNORM;
            case(R8G8B8A8_SINT): return VK_FORMAT_R8G8B8A8_SINT;
            case(R8G8B8A8_UINT): return VK_FORMAT_R8G8B8A8_UINT;
            case(R8G8B8A8_UNORM_SRGB): return VK_FORMAT_R8G8B8A8_SRGB;

            case(B8G8R8A8_UNORM): return VK_FORMAT_B8G8R8A8_UNORM;  
            case(B8G8R8A8_UNORM_SRGB): return VK_FORMAT_B8G8R8A8_SRGB;
            
            case(R16_UNORM): return VK_FORMAT_R16_UNORM;
            case(R16_SNORM): return VK_FORMAT_R16_SNORM;
            case(R16_SINT): return VK_FORMAT_R16_SINT;
            case(R16_UINT): return VK_FORMAT_R16_UINT;
            case(R16_FLOAT): return VK_FORMAT_R16_SFLOAT;

            case(R16G16_UNORM): return VK_FORMAT_R16G16_UNORM;
            case(R16G16_SNORM): return VK_FORMAT_R16G16_SNORM;
            case(R16G16_SINT): return VK_FORMAT_R16G16_SINT;
            case(R16G16_UINT): return VK_FORMAT_R16G16_UINT;
            case(R16G16_FLOAT): return VK_FORMAT_R16G16_SFLOAT;

            case(R16G16B16A16_FLOAT): return VK_FORMAT_R16G16B16A16_SFLOAT;
            case(R16G16B16A16_UNORM): return VK_FORMAT_R16G16B16A16_UNORM;
            case(R16G16B16A16_SNORM): return VK_FORMAT_R16G16B16A16_SNORM;
            case(R16G16B16A16_SINT): return VK_FORMAT_R16G16B16A16_SINT;
            case(R16G16B16A16_UINT): return VK_FORMAT_R16G16B16A16_UINT;

            case(R32_UINT): return VK_FORMAT_R32_UINT;
            case(R32_SINT): return VK_FORMAT_R32_SINT;
            case(R32_FLOAT): return VK_FORMAT_R32_SFLOAT;

            case(R32G32_UINT): return VK_FORMAT_R32G32_UINT;
            case(R32G32_SINT): return VK_FORMAT_R32G32_SINT;
            case(R32G32_FLOAT): return VK_FORMAT_R32G32_SFLOAT;
            
            case(R32G32B32_UINT): return VK_FORMAT_R32G32B32_UINT;
            case(R32G32B32_SINT): return VK_FORMAT_R32G32B32_SINT;
            case(R32G32B32_FLOAT): return VK_FORMAT_R32G32B32_SFLOAT;
            
            case(R32G32B32A32_UINT): return VK_FORMAT_R32G32B32A32_UINT;
            case(R32G32B32A32_SINT): return VK_FORMAT_R32G32B32A32_SINT;
            case(R32G32B32A32_FLOAT): return VK_FORMAT_R32G32B32A32_SFLOAT;

            case(D32_FLOAT): return VK_FORMAT_D32_SFLOAT;
            case(D24_UNORM_S8_UINT):return VK_FORMAT_D24_UNORM_S8_UINT;
            case(D16_UNORM): return VK_FORMAT_D16_UNORM;
            case(D32_FLOAT_S8X24_UINT): return VK_FORMAT_D32_SFLOAT_S8_UINT;

            case(BC1_UNORM): return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            case(BC1_UNORM_SRGB): return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            case(BC2_UNORM): return VK_FORMAT_BC2_UNORM_BLOCK;  
            case(BC2_UNORM_SRGB): return VK_FORMAT_BC2_SRGB_BLOCK;
            case(BC3_UNORM): return VK_FORMAT_BC3_UNORM_BLOCK;  
            case(BC3_UNORM_SRGB): return VK_FORMAT_BC3_SRGB_BLOCK;
            case(BC4_UNORM): return VK_FORMAT_BC4_UNORM_BLOCK;  
            case(BC4_SNORM): return VK_FORMAT_BC4_SNORM_BLOCK;
            case(BC5_UNORM): return VK_FORMAT_BC5_UNORM_BLOCK;  
            case(BC5_SNORM): return VK_FORMAT_BC5_SNORM_BLOCK;
            case(BC6H_UF16): return VK_FORMAT_BC6H_UFLOAT_BLOCK;
            case(BC6H_SF16): return VK_FORMAT_BC6H_SFLOAT_BLOCK;
            case(BC7_UNORM): return VK_FORMAT_BC7_UNORM_BLOCK;
            case(BC7_UNORM_SRGB): return VK_FORMAT_BC7_SRGB_BLOCK;
            default: return VK_FORMAT_UNDEFINED;
              break;
            }
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
            break;
        case RHI::PrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            break;
        case RHI::PrimitiveTopology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            break;
        case RHI::PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            break;
        case RHI::PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            break;
        default:
            break;
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
        for (int i = 0; i < queueFamilyCount; i++)
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