#include "CommandAllocator.h"
#include "CommandList.h"
#include "CommandQueue.h"
#include "DebugBuffer.h"
#include "DescriptorHeap.h"
#include "FormatsAndTypes.h"
#include "Heap.h"
#include "Instance.h"
#include "PipelineStateObject.h"
#include "Ptr.h"
#include "RootSignature.h"
#include "Texture.h"
#include "TextureView.h"
#include "pch.h"
#include "../Device.h"
#include "../../Error.h"
#include "VulkanSpecific.h"
#include "result.hpp"
#include "volk.h"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>
#define VMA_IMPLEMENTATION
#define VULKAN_AFTER_CRASH_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#include "VulkanAfterCrash.h"
#include "vk_mem_alloc.h"
#include <iostream>
#include <fstream>
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))
#endif
using namespace ezr;
static void SelectHeapIndices(RHI::Weak<RHI::vDevice> device)
{
    std::uint32_t DefaultHeap = UINT32_MAX;
    std::uint32_t UploadHeap = UINT32_MAX;
    std::uint32_t ReadbackHeap = UINT32_MAX;
    uint32_t iterator = 0;
    for (auto& prop : device->HeapProps)
    {
        if ((prop.memoryLevel ==  RHI::MemoryLevel::DedicatedRAM) && (iterator < DefaultHeap)) DefaultHeap = iterator;
        if (prop.pageProperty == RHI::CPUPageProperty::WriteCached) UploadHeap = iterator;
        if ((prop.pageProperty == RHI::CPUPageProperty::WriteCombined) && (iterator < UploadHeap)) UploadHeap = iterator;
        if ((prop.pageProperty == RHI::CPUPageProperty::WriteCombined) || ((prop.pageProperty == RHI::CPUPageProperty::WriteCached) && (iterator < ReadbackHeap))) ReadbackHeap = iterator;
        iterator++;
    }
    device->DefaultHeapIndex = DefaultHeap;
    device->UploadHeapIndex = UploadHeap;
    device->ReadBackHeapIndex = ReadbackHeap;
}

namespace RHI
{
    ezr::result<std::pair<Ptr<Device>, std::vector<Ptr<CommandQueue>>>, CreationError>
    Device::Create(PhysicalDevice* PhysicalDevice,std::span<CommandQueueDesc> commandQueueInfos, Internal_ID instance, DeviceCreateFlags flags)
    {
        uint32_t numCommandQueues = commandQueueInfos.size();
        auto vkPhysicalDevice = static_cast<VkPhysicalDevice>(PhysicalDevice->ID);
        VkPhysicalDeviceMemoryProperties memProps;
        Ptr<vDevice> vdevice(new RHI::vDevice);
        std::vector<Ptr<CommandQueue>> vqueue;
        vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProps);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        {
            VkFlags mem_flags = memProps.memoryTypes[i].propertyFlags;
            RHI::HeapProperties prop;
            prop.type = RHI::HeapType::Custom;
            prop.memoryLevel = (mem_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ? RHI::MemoryLevel::DedicatedRAM : RHI::MemoryLevel::SharedRAM;
            RHI::CPUPageProperty CPUprop;
            if (mem_flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) CPUprop = RHI::CPUPageProperty::WriteCached;
            else if (mem_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) CPUprop = RHI::CPUPageProperty::WriteCombined;
            else CPUprop = RHI::CPUPageProperty::NonVisible;
            prop.pageProperty = CPUprop;
            vdevice->HeapProps.emplace_back(prop);
        }
        SelectHeapIndices(vdevice);
        auto [qfamilies, qcounts] = findQueueFamilyIndices(PhysicalDevice);
        vdevice->indices = qfamilies;
        std::vector<uint32_t> usedQueues(qcounts.size(), 0);

        std::unordered_map<uint32_t, VkDeviceQueueCreateInfo> queueInfos;
        std::unordered_map<uint32_t, std::vector<float>> priorities;
        for (auto& qi:commandQueueInfos)
        {
            std::uint32_t index = 0;
            if (qi.commandListType == RHI::CommandListType::Direct) index = vdevice->indices.graphicsIndex;
            else if (qi.commandListType == RHI::CommandListType::Compute) index = vdevice->indices.computeIndex;
            else if (qi.commandListType == RHI::CommandListType::Copy) index = vdevice->indices.copyIndex;
            else return ezr::err(CreationError::CommandQueueNotAvailable);
            if (qcounts[index] <= usedQueues[index])
            {
                RHI::log(LogLevel::Error, std::format("Attempted to Create Too Many Queues of type {0}", [&]{
                    if (qi.commandListType == RHI::CommandListType::Direct) return "Graphics";
                    else if (qi.commandListType == RHI::CommandListType::Compute) return "Compute";
                    else if (qi.commandListType == RHI::CommandListType::Copy) return "Copy";
                    else return "Unkown";
                }()));
                return ezr::err(CreationError::CommandQueueNotAvailable);
            }

            auto& qInfo = queueInfos[index];
            auto& queuePriority = priorities[index];
            queuePriority.push_back(qi.Priority);

            qInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qInfo.pNext = nullptr;
            qInfo.pQueuePriorities = queuePriority.data();
            qi._unused = usedQueues[index]++;
            qInfo.queueCount = usedQueues[index];
            qInfo.queueFamilyIndex = index;
            qInfo.flags = 0;
        }
        if (vdevice->indices.graphicsIndex != vdevice->indices.presentIndex)
        {
            VkDeviceQueueCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            float queuePriority = 0.f;
            info.pQueuePriorities = &queuePriority;
            info.queueCount = 1;
            info.queueFamilyIndex = vdevice->indices.presentIndex;
            info.flags = 0;
            queueInfos[vdevice->indices.presentIndex] = info;
        }
        std::vector<VkDeviceQueueCreateInfo> queueInfo_vec(queueInfos.size());
        std::ranges::copy(std::ranges::views::values(queueInfos), queueInfo_vec.begin());
        VkPhysicalDeviceFeatures deviceFeatures{};
        //deviceFeatures.shaderStorageImageReadWithoutFormat = VK_TRUE;
        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
        dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

        VkPhysicalDeviceTimelineSemaphoreFeatures semaphoreFeatures{};
        semaphoreFeatures.timelineSemaphore = VK_TRUE;
        semaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
        semaphoreFeatures.pNext = &dynamicRenderingFeatures;

        VkDeviceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.pNext = &semaphoreFeatures;
        info.pQueueCreateInfos = queueInfo_vec.data();
        info.queueCreateInfoCount = queueInfo_vec.size();
        info.pEnabledFeatures = &deviceFeatures;
        const char* ext_name[] =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
            VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
            VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
            VK_KHR_MAINTENANCE1_EXTENSION_NAME,
            VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
            VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME,
            VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
            #ifdef WIN32
            VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
            #endif
        };
        info.enabledExtensionCount = ARRAYSIZE(ext_name);
        info.ppEnabledExtensionNames = ext_name;
        VkResult res = vkCreateDevice(vkPhysicalDevice, &info, nullptr, reinterpret_cast<VkDevice*>(&vdevice->ID));
        if(res < 0)
        {
            return ezr::err(marshall_error(res));
        }
        vqueue.reserve(numCommandQueues);
        for (uint32_t i = 0; i < numCommandQueues; i++)
        {
            vqueue.emplace_back(new vCommandQueue);
            uint32_t index = 0;
            if (commandQueueInfos[i].commandListType == RHI::CommandListType::Direct) index = vdevice->indices.graphicsIndex;
            if (commandQueueInfos[i].commandListType == RHI::CommandListType::Compute) index = vdevice->indices.computeIndex;
            if (commandQueueInfos[i].commandListType == RHI::CommandListType::Copy) index = vdevice->indices.copyIndex;
            vkGetDeviceQueue(static_cast<VkDevice>(vdevice->ID), index, commandQueueInfos[i]._unused, reinterpret_cast<VkQueue*>(&vqueue[i]->ID));
        }
        //initialize VMA
        VkExternalMemoryHandleTypeFlagsKHR ext_mem[] = {
            #ifdef WIN32
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
            #endif // WIN32

        };
        VmaAllocatorCreateInfo vmaInfo{};
        vmaInfo.device = static_cast<VkDevice>(vdevice->ID);
        vmaInfo.flags = 0; //probably have error checking
        vmaInfo.instance = static_cast<VkInstance>(instance);
        vmaInfo.physicalDevice = static_cast<VkPhysicalDevice>(PhysicalDevice->ID);
        vmaInfo.pVulkanFunctions = nullptr;
        vmaInfo.pTypeExternalMemoryHandleTypes = (flags & RHI::DeviceCreateFlags::ShareAutomaticMemory) != RHI::DeviceCreateFlags::None ? ext_mem:nullptr;//

        res = vmaCreateAllocator(&vmaInfo, &vdevice->allocator);
        if(res < 0) return ezr::err(marshall_error(res));
        VkAfterCrash_DeviceCreateInfo acInfo;
        acInfo.flags = 0;
        acInfo.vkDevice = static_cast<VkDevice>(vdevice->ID);
        acInfo.vkPhysicalDevice = static_cast<VkPhysicalDevice>(PhysicalDevice->ID);
        VkAfterCrash_CreateDevice(&acInfo, &vdevice->acDevice);
        return ezr::ok(std::pair<Ptr<Device>, std::vector<Ptr<CommandQueue>>>{vdevice.transform<Device>(), vqueue});
    }
    creation_result<Device> Device::FromNativeHandle(Internal_ID id, Internal_ID phys_device, Internal_ID instance, const QueueFamilyIndices& indices)
    {
        auto vkPhysicalDevice = static_cast<VkPhysicalDevice>(phys_device);
        VkPhysicalDeviceMemoryProperties memProps;
        Ptr<vDevice> vdevice(new RHI::vDevice);
        vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProps);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        {
            VkFlags flags = memProps.memoryTypes[i].propertyFlags;
            RHI::HeapProperties prop;
            prop.type = RHI::HeapType::Custom;
            prop.memoryLevel = (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ? RHI::MemoryLevel::DedicatedRAM : RHI::MemoryLevel::SharedRAM;
            RHI::CPUPageProperty CPUprop;
            if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) CPUprop = RHI::CPUPageProperty::WriteCached;
            else if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) CPUprop = RHI::CPUPageProperty::WriteCombined;
            else CPUprop = RHI::CPUPageProperty::NonVisible;
            prop.pageProperty = CPUprop;
            vdevice->HeapProps.emplace_back(prop);
        }
        SelectHeapIndices(Weak<vDevice>{vdevice});
        vdevice->indices = indices;
        vdevice->ID = id;
        VmaAllocatorCreateInfo vmaInfo{};
        vmaInfo.device = static_cast<VkDevice>(vdevice->ID);
        vmaInfo.flags = 0; //probably have error checking
        vmaInfo.instance = static_cast<VkInstance>(instance);
        vmaInfo.physicalDevice = vkPhysicalDevice;
        vmaInfo.pVulkanFunctions = nullptr;
        vmaInfo.pTypeExternalMemoryHandleTypes = nullptr;

        VkResult res = vmaCreateAllocator(&vmaInfo, &vdevice->allocator);
        if(res < 0) return ezr::err(marshall_error(res));
        VkAfterCrash_DeviceCreateInfo acInfo;
        acInfo.flags = 0;
        acInfo.vkDevice = static_cast<VkDevice>(vdevice->ID);
        acInfo.vkPhysicalDevice = vkPhysicalDevice;
        res = VkAfterCrash_CreateDevice(&acInfo, &vdevice->acDevice);
        if(res < 0) return ezr::err(marshall_error(res));
        vdevice->Hold();//the device doesn't belong to us, so we keep it alive for the external user
        return ezr::ok(vdevice.transform<Device>());
    }
    Default_t Default = {};
    Zero_t Zero = {};
    static CreationError CreateShaderModule(const char* filename, VkPipelineShaderStageCreateInfo* shader_info, VkShaderStageFlagBits stage,int index, VkShaderModule* module,Internal_ID device)
    {
        if(!std::filesystem::exists(filename))
        {
            return CreationError::FileNotFound;
        }
        std::vector<char> buffer;
        std::ifstream file(filename, std::ios::binary);
        uint32_t spvSize;
        file.read(reinterpret_cast<char*>(&spvSize), 4);
        if constexpr  (std::endian::native != std::endian::little)
        {
            std::span size_bytes = std::as_writable_bytes(std::span{&spvSize, 1});
            std::ranges::reverse(size_bytes);
        }
        if(spvSize == 0) return CreationError::IncompatibleShader;
        buffer.resize(spvSize);
        file.read(buffer.data(), spvSize);

        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = buffer.size();
        info.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
        if(auto res = vkCreateShaderModule(static_cast<VkDevice>(device), &info, nullptr, &module[index]); res < VK_SUCCESS)
            return marshall_error(res);
        auto& [sType, pNext, flags, stg, md, pName, pSpecializationInfo] = shader_info[index];
        sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stg = stage;
        md = module[index];
        pName = "main";

        return CreationError::None;
    }
    static CreationError CreateShaderModule(const char* memory, uint32_t size, VkPipelineShaderStageCreateInfo* shader_info, VkShaderStageFlagBits stage, int index, VkShaderModule* module, Internal_ID device)
    {
        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = size;
        info.pCode = reinterpret_cast<const uint32_t*>(memory);
        if(const auto res = vkCreateShaderModule(static_cast<VkDevice>(device), &info, nullptr, &module[index]); res < VK_SUCCESS)
            return marshall_error(res);
        auto& [sType, pNext, flags, stg, md, pName, pSpecializationInfo] = shader_info[index];
        sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stg = stage;
        md = module[index];
        pName = "main";
        return CreationError::None;
    }
    VkExternalMemoryHandleTypeFlagBitsKHR MemFlags(ExportOptions opt)
    {
        switch (opt)
        {
        case RHI::ExportOptions::D3D11TextureNT:return VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT_KHR;
        case RHI::ExportOptions::Win32Handle:return VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
        case RHI::ExportOptions::FD:return VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
        default:return VK_EXTERNAL_MEMORY_HANDLE_TYPE_FLAG_BITS_MAX_ENUM;
        }
    }
    RESULT Device::GetMemorySharingCapabilites()
    {
        return 0;
    }

    RESULT Device::ExportTexture(Texture* texture, ExportOptions options, MemHandleT* handle)
    {
        auto* vtex = reinterpret_cast<vTexture*>(texture);
        #ifdef WIN32
        VkMemoryGetWin32HandleInfoKHR info;
        info.memory = vtex->vma_ID ? vtex->vma_ID->GetMemory() : (VkDeviceMemory)vtex->heap;
        info.pNext = 0;
        info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        info.handleType = MemFlags(options);
        return vkGetMemoryWin32HandleKHR((VkDevice)ID, &info, handle);
        #endif
        VkMemoryGetFdInfoKHR info;
        info.handleType = MemFlags(options);
        info.memory = vtex->vma_ID ? vtex->vma_ID->GetMemory() : reinterpret_cast<VkDeviceMemory>(vtex->heap.Get());
        info.pNext = nullptr;
        info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        return vkGetMemoryFdKHR(static_cast<VkDevice>(ID),&info,handle);

    }
    RESULT Device::QueueWaitIdle(Weak<CommandQueue> queue)
    {
        return vkQueueWaitIdle(static_cast<VkQueue>(queue->ID));
    }
    static VkBufferUsageFlags VkBufferUsage(RHI::BufferUsage usage)
    {
        VkBufferUsageFlags flags = 0;
        if ((usage & RHI::BufferUsage::VertexBuffer) != BufferUsage::None)
        {
            flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if ((usage & RHI::BufferUsage::ConstantBuffer) != BufferUsage::None)
        {
            flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        if ((usage & RHI::BufferUsage::IndexBuffer) != BufferUsage::None)
        {
            flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if ((usage & RHI::BufferUsage::CopySrc) != BufferUsage::None)
        {
            flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
        if ((usage & RHI::BufferUsage::CopyDst) != BufferUsage::None)
        {
            flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        if ((usage & RHI::BufferUsage::StructuredBuffer) != BufferUsage::None)
        {
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        return flags;
    }
    creation_result<CommandAllocator> Device::CreateCommandAllocator(CommandListType type)
    {
        Ptr<vCommandAllocator> vallocator(new vCommandAllocator);
        std::uint32_t index = 0;
        if (type == CommandListType::Direct) index = reinterpret_cast<vDevice*>(this)->indices.graphicsIndex;
        if (type == CommandListType::Compute) index = reinterpret_cast<vDevice*>(this)->indices.computeIndex;
        if (type == CommandListType::Copy) index = reinterpret_cast<vDevice*>(this)->indices.copyIndex;
        VkCommandPoolCreateInfo info{};
        info.pNext = nullptr;
        info.queueFamilyIndex = index;
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; //to align with d3d's model
        VkResult res = vkCreateCommandPool(static_cast<VkDevice>(ID), &info, nullptr, reinterpret_cast<VkCommandPool*>(&vallocator->ID));
        vallocator->device = make_ptr(this);


        if(res < 0) return creation_result<CommandAllocator>::err(marshall_error(res));
        return creation_result<CommandAllocator>::ok(vallocator);
    }
    creation_result<GraphicsCommandList> Device::CreateCommandList(CommandListType type, const Ptr<CommandAllocator>& allocator)
    {
        Ptr<vGraphicsCommandList> vCommandlist(new vGraphicsCommandList);
        VkCommandBufferAllocateInfo Info = {};
        Info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        Info.commandPool = static_cast<VkCommandPool>(allocator->ID);
        Info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        Info.commandBufferCount = 1;
        VkCommandBuffer commandBuffer;
        const VkResult res = vkAllocateCommandBuffers(static_cast<VkDevice>(ID), &Info, &commandBuffer);
        vCommandlist->ID = commandBuffer;
        vCommandlist->device = make_ptr(this);
        vCommandlist->allocator = allocator.transform<vCommandAllocator>();

        if(res < 0)
        {
            return creation_result<GraphicsCommandList>::err(marshall_error(res));
        }

        allocator.retrieve_as_forced<vCommandAllocator>()->m_pools.push_back(vCommandlist->ID);

        return creation_result<GraphicsCommandList>::ok(vCommandlist);
    }
    VkDescriptorType DescType(RHI::DescriptorType type)
    {
        using enum DescriptorType;
        switch (type)
        {
        case SampledTexture: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case ConstantBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case StructuredBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case ConstantBufferDynamic: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case StructuredBufferDynamic: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        case Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
        case CSTexture: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case CSBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        default: return VK_DESCRIPTOR_TYPE_MAX_ENUM;
        }
    }
    creation_result<DescriptorHeap> Device::CreateDescriptorHeap(const DescriptorHeapDesc& desc)
    {
        Ptr<vDescriptorHeap> vdescriptorHeap(new vDescriptorHeap);
        VkResult res = VK_SUCCESS;
        if (desc.poolSizes->type == DescriptorType::RTV || desc.poolSizes->type == DescriptorType::DSV)
        {
            vdescriptorHeap->ID = new VkImageView[desc.poolSizes->numDescriptors];
            vdescriptorHeap->type = RHI::DescriptorType::RTV;//this is purely for correct deletion
            const auto begin = static_cast<VkImageView*>(vdescriptorHeap->ID);
            for (auto& elem : std::ranges::subrange(begin, begin + desc.poolSizes->numDescriptors)) elem = nullptr;
            vdescriptorHeap->num_descriptors = desc.poolSizes->numDescriptors;
        }
        else if (desc.poolSizes->type == DescriptorType::Sampler)
        {
            vdescriptorHeap->ID = new VkSampler[desc.poolSizes->numDescriptors];
            vdescriptorHeap->type = RHI::DescriptorType::Sampler;
            const auto begin = static_cast<VkSampler*>(vdescriptorHeap->ID);
            for (auto& elem : std::ranges::subrange(begin, begin + desc.poolSizes->numDescriptors)) elem = nullptr;
            vdescriptorHeap->num_descriptors = desc.poolSizes->numDescriptors;
        }
        else
        {
            vdescriptorHeap->type = RHI::DescriptorType::ConstantBuffer;//again this is not supposed to be correct
            VkDescriptorPoolSize poolSize[5]{};
            for (uint32_t i = 0; i < desc.numPoolSizes; i++)
            {
                poolSize[i].type = DescType(desc.poolSizes[i].type);
                poolSize[i].descriptorCount = desc.poolSizes[i].numDescriptors;
            }

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = desc.numPoolSizes;
            poolInfo.pPoolSizes = poolSize;
            poolInfo.maxSets = desc.maxDescriptorSets;

            res = vkCreateDescriptorPool(static_cast<VkDevice>(ID), &poolInfo, nullptr, reinterpret_cast<VkDescriptorPool*>(&vdescriptorHeap->ID));
        }
        vdescriptorHeap->device = make_ptr(this);
        if(res < 0) return creation_result<DescriptorHeap>::err(marshall_error(res));
        return creation_result<DescriptorHeap>::ok(vdescriptorHeap);
    }
    CreationError Device::CreateRenderTargetView(const Weak<Texture> texture, const RenderTargetViewDesc& desc, const CPU_HANDLE heapHandle) const
    {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.pNext = nullptr;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.image = static_cast<VkImage>(texture->ID);
        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = desc.textureMipSlice;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = desc.TextureArray ? desc.arraySlice: 0;
        info.subresourceRange.layerCount = 1;
        info.format = FormatConv(desc.format);
       return marshall_error(vkCreateImageView(static_cast<VkDevice>(ID), &info, nullptr, static_cast<VkImageView*>(heapHandle.ptr)));

    }
    CreationError Device::CreateDepthStencilView(const Weak<Texture> texture, const DepthStencilViewDesc& desc, const CPU_HANDLE heapHandle) const
    {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.pNext = nullptr;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.image = static_cast<VkImage>(texture->ID);
        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        info.subresourceRange.baseMipLevel = desc.textureMipSlice;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = desc.TextureArray ? desc.arraySlice:0;
        info.subresourceRange.layerCount = 1;
        info.format = FormatConv(desc.format);
        return marshall_error(vkCreateImageView(static_cast<VkDevice>(ID), &info, nullptr, static_cast<VkImageView*>(heapHandle.ptr)));
    }
    std::uint32_t Device::GetDescriptorHeapIncrementSize(DescriptorType type)
    {
        if(type == RHI::DescriptorType::RTV || type == RHI::DescriptorType::DSV)
            return sizeof(VkImageView);
        if (type == RHI::DescriptorType::Sampler)
            return sizeof(VkSampler);
        return 0;
    }
    creation_result<Texture> Device::GetSwapChainImage(Weak<SwapChain> swapchain, std::uint32_t index)
    {
        Ptr<vTexture> vtexture(new vTexture);
        std::uint32_t img = index + 1;
        std::vector<VkImage> images(img);
        VkResult res = vkGetSwapchainImagesKHR(static_cast<VkDevice>(ID), static_cast<VkSwapchainKHR>(swapchain->ID), &img, images.data());
        vtexture->ID = images[index];
        vtexture->Hold();//the image is externally owned
        vtexture->device = make_ptr(this);
        if(res < 0) return creation_result<Texture>::err(marshall_error(res));
        return ezr::ok(vtexture.transform<Texture>());
    }
    creation_result<Fence> Device::CreateFence( std::uint64_t val)
    {
        Ptr<vFence> vfence(new vFence);
        VkSemaphoreTypeCreateInfo timelineCreateInfo;
        timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timelineCreateInfo.pNext = nullptr;
        timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineCreateInfo.initialValue = val;

        VkSemaphoreCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        createInfo.pNext = &timelineCreateInfo;
        createInfo.flags = 0;

        VkSemaphore timelineSemaphore;
        const VkResult res = vkCreateSemaphore(static_cast<VkDevice>(ID), &createInfo, nullptr, &timelineSemaphore);
        vfence->ID = timelineSemaphore;
        vfence->device = make_ptr(this);

        if(res < 0) return creation_result<Fence>::err(marshall_error(res));
        return creation_result<Fence>::ok(vfence);
    }
    creation_result<Buffer> Device::CreateBuffer(const BufferDesc& desc, const Ptr<Heap>& heap, const HeapProperties* props, const AutomaticAllocationInfo* automatic_info, const std::uint64_t offset, const ResourceType type)
    {
        Ptr<vBuffer> vbuffer(new vBuffer);
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.size = desc.size;
        info.usage = VkBufferUsage(desc.usage);
        vbuffer->device = make_ptr(this);
        if (type == ResourceType::Automatic)
        {
            VmaAllocationCreateInfo allocCreateInfo{};
            allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocCreateInfo.flags = 0;
            allocCreateInfo.flags |=
                (automatic_info->access_mode == AutomaticAllocationCPUAccessMode::Random) ? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT : 0;
            allocCreateInfo.flags |=
                (automatic_info->access_mode == AutomaticAllocationCPUAccessMode::Sequential) ? VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT : 0;
            VkResult res = vmaCreateBuffer(reinterpret_cast<vDevice*>(this)->allocator, &info, &allocCreateInfo, reinterpret_cast<VkBuffer*>(&vbuffer->ID), &vbuffer->vma_ID, nullptr);

            if(res < 0) creation_result<Buffer>::err(marshall_error(res));
            return creation_result<Buffer>::ok(vbuffer);
        }
        vbuffer->offset = offset;
        vbuffer->size = desc.size;
        VkResult res = vkCreateBuffer(static_cast<VkDevice>(ID), &info, nullptr, reinterpret_cast<VkBuffer*>(&vbuffer->ID));
        if(res < 0) return creation_result<Buffer>::err(marshall_error(res));
        if (type == ResourceType::Commited)
        {
            MemoryRequirements req = GetBufferMemoryRequirements(desc);
            auto r = CreateHeap(HeapDesc{req.size, *props}, nullptr);
            if(r.is_err()) return r.transform([](Ptr<Heap>&)->Ptr<Buffer>{return nullptr;});
            vbuffer->heap = r.value();
            res = vkBindBufferMemory(static_cast<VkDevice>(ID), static_cast<VkBuffer>(vbuffer->ID), static_cast<VkDeviceMemory>(vbuffer->heap->ID), 0);
            if(res < 0) return creation_result<Buffer>::err(marshall_error(res));
        }
        else if (type == ResourceType::Placed)
        {
            vbuffer->heap = heap;
            res = vkBindBufferMemory(static_cast<VkDevice>(ID), static_cast<VkBuffer>(vbuffer->ID), static_cast<VkDeviceMemory>(heap->ID), offset);
            if(res < 0) return creation_result<Buffer>::err(marshall_error(res));
        }

        return creation_result<Buffer>::ok(vbuffer);
    }
    MemoryRequirements Device::GetBufferMemoryRequirements(const BufferDesc& desc)
    {
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.size = desc.size;
        info.usage = desc.usage == BufferUsage::VertexBuffer ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VkDeviceBufferMemoryRequirements DeviceReq{};
        DeviceReq.sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS;
        DeviceReq.pCreateInfo = &info;
        VkMemoryRequirements2 req{};
        req.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        vkGetDeviceBufferMemoryRequirementsKHR(static_cast<VkDevice>(ID), &DeviceReq, &req);
        const MemoryRequirements requirements {
            .size = req.memoryRequirements.size,
            .alignment = req.memoryRequirements.alignment,
            .memoryTypeBits = req.memoryRequirements.memoryTypeBits,
        };
        return requirements;
    }
    creation_result<Heap> Device::CreateHeap(const HeapDesc& desc, bool* usedFallback)
    {
        Ptr<vHeap> vheap(new vHeap);
        if(usedFallback)*usedFallback = false;
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = desc.size;
        uint32_t MemIndex = UINT32_MAX;
        if (desc.props.type == HeapType::Custom)
        {
            uint32_t iterator = 0;
            for (const auto& prop : reinterpret_cast<vDevice*>(this)->HeapProps)
            {
                if (prop.memoryLevel != desc.props.memoryLevel) { iterator++; continue; }
                if (prop.pageProperty != CPUPageProperty::NonVisible && desc.props.pageProperty == CPUPageProperty::Any)
                {
                    MemIndex = iterator;
                    iterator++;
                    continue;
                }
                if (prop.pageProperty != desc.props.pageProperty) {iterator++; continue;}
            }
            if (MemIndex == UINT32_MAX)
            {
                if(usedFallback)*usedFallback = true;
                if (desc.props.FallbackType == HeapType::Default)  MemIndex = reinterpret_cast<vDevice*>(this)->DefaultHeapIndex;
                if (desc.props.FallbackType == HeapType::Upload)   MemIndex = reinterpret_cast<vDevice*>(this)->UploadHeapIndex;
                if (desc.props.FallbackType == HeapType::ReadBack) MemIndex = reinterpret_cast<vDevice*>(this)->ReadBackHeapIndex;
            }
        }
        else
        {
            if (desc.props.type == HeapType::Default)  MemIndex = reinterpret_cast<vDevice*>(this)->DefaultHeapIndex;
            if (desc.props.type == HeapType::Upload)   MemIndex = reinterpret_cast<vDevice*>(this)->UploadHeapIndex;
            if (desc.props.type == HeapType::ReadBack) MemIndex = reinterpret_cast<vDevice*>(this)->ReadBackHeapIndex;
        }
        allocInfo.memoryTypeIndex = MemIndex;
        VkResult res = vkAllocateMemory(static_cast<VkDevice>(ID), &allocInfo, nullptr, reinterpret_cast<VkDeviceMemory*>(&vheap->ID));
        if(res < 0) return creation_result<Heap>::err(marshall_error(res));

        return creation_result<Heap>::ok(vheap);
    }
    VkShaderStageFlags VkShaderStage(ShaderStage stage)
    {
        VkShaderStageFlags flags = 0;
        if ((stage & ShaderStage::Vertex) != ShaderStage::None)
        {
            flags |= VK_SHADER_STAGE_VERTEX_BIT;
        }
        if ((stage & ShaderStage::Pixel) != ShaderStage::None)
        {
            flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        if ((stage & ShaderStage::Geometry) != ShaderStage::None)
        {
            flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
        }
        if ((stage & ShaderStage::Hull) != ShaderStage::None)
        {
            flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }
        if ((stage & ShaderStage::Domain) != ShaderStage::None)
        {
            flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        }
        if ((stage & ShaderStage::Compute) != ShaderStage::None)
        {
            flags |= VK_SHADER_STAGE_COMPUTE_BIT;
        }
        return flags;
    }
    creation_result<RootSignature> Device::CreateRootSignature(const RootSignatureDesc& desc, Ptr<DescriptorSetLayout>* pSetLayouts)
    {
        Ptr<vRootSignature> vrootSignature(new vRootSignature);
        std::tuple<VkDescriptorSetLayout, RootParameterType, uint32_t> descriptorSetLayout[20];
        VkPushConstantRange pushConstantRanges[20];

        VkResult res;

        //uint32_t minSetIndex = UINT32_MAX;
        uint32_t numLayouts = 0;
        uint32_t pcIndex = 0;
        for (auto& param : desc.rootParameters)
        {
            if (param.type == RootParameterType::DynamicDescriptor)
            {
                VkDescriptorSetLayoutCreateInfo layoutInfo{};
                if(param.dynamicDescriptor.type != DescriptorType::ConstantBufferDynamic
                && param.dynamicDescriptor.type != DescriptorType::StructuredBufferDynamic)
                {
                    for(uint32_t j = 0; j < numLayouts; j++)
                    {
                        if(std::get<0>(descriptorSetLayout[j]))
                            vkDestroyDescriptorSetLayout(static_cast<VkDevice>(ID), std::get<0>(descriptorSetLayout[j]), nullptr);
                    }
                    return ezr::err(CreationError::InvalidParameters);
                }

                VkDescriptorSetLayoutBinding binding;
                binding.binding = 0;
                binding.descriptorCount = 1;
                binding.descriptorType = DescType(param.dynamicDescriptor.type);
                binding.pImmutableSamplers = nullptr;
                binding.stageFlags = VkShaderStage(param.dynamicDescriptor.stage);
                layoutInfo.bindingCount = 1;
                layoutInfo.pBindings = &binding;
                layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                res = vkCreateDescriptorSetLayout(static_cast<VkDevice>(ID), &layoutInfo, nullptr,
                    &std::get<0>(descriptorSetLayout[numLayouts]));
                if(res < 0)
                {
                    for(uint32_t j = 0; j < numLayouts; j++)
                    {
                        if(std::get<0>(descriptorSetLayout[j])) 
                            vkDestroyDescriptorSetLayout(static_cast<VkDevice>(ID), std::get<0>(descriptorSetLayout[j]), nullptr);
                    }
                    return creation_result<RootSignature>::err(marshall_error(res));
                }
                std::get<1>(descriptorSetLayout[numLayouts]) = RootParameterType::DynamicDescriptor;
                std::get<2>(descriptorSetLayout[numLayouts]) = param.dynamicDescriptor.setIndex;
                numLayouts++;
            }
            else if(param.type == RootParameterType::PushConstant)
            {
                pushConstantRanges[pcIndex].offset = param.pushConstant.offset;
                pushConstantRanges[pcIndex].size = param.pushConstant.numConstants * sizeof(uint32_t);
                pushConstantRanges[pcIndex].stageFlags = vrootSignature->pcBindingToStage[param.pushConstant.bindingIndex] =
                VkShaderStage(param.pushConstant.stage);
                pcIndex++;
            }
            else if(param.type == RootParameterType::DescriptorTable)
            {
                VkDescriptorSetLayoutCreateInfo layoutInfo{};
                VkDescriptorSetLayoutBinding LayoutBinding[20] = {};
                for (uint32_t j = 0; j < param.descriptorTable.ranges.size(); j++)
                {
                    LayoutBinding[j].binding = param.descriptorTable.ranges[j].BaseShaderRegister;
                    LayoutBinding[j].descriptorCount = param.descriptorTable.ranges[j].numDescriptors;
                    LayoutBinding[j].descriptorType = DescType(param.descriptorTable.ranges[j].type);
                    LayoutBinding[j].pImmutableSamplers = nullptr;
                    LayoutBinding[j].stageFlags = VkShaderStage(param.descriptorTable.ranges[j].stage);
                }
                layoutInfo.bindingCount = param.descriptorTable.ranges.size();
                layoutInfo.pBindings = LayoutBinding;
                layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                res = vkCreateDescriptorSetLayout(static_cast<VkDevice>(ID), &layoutInfo, nullptr,
                &std::get<0>(descriptorSetLayout[numLayouts]));
                if(res < 0)
                {
                    for(uint32_t j = 0; j < numLayouts; j++)
                    {
                        if(std::get<0>(descriptorSetLayout[j])) 
                            vkDestroyDescriptorSetLayout(static_cast<VkDevice>(ID), std::get<0>(descriptorSetLayout[j]), nullptr);
                    }
                    return creation_result<RootSignature>::err(marshall_error(res));
                }
                std::get<1>(descriptorSetLayout[numLayouts]) = RootParameterType::DescriptorTable;
                std::get<2>(descriptorSetLayout[numLayouts]) = param.descriptorTable.setIndex;
                numLayouts++;
            }
        }
        VkDescriptorSetLayout layoutInfoSorted[20]{};
        //sort descriptor sets
        for (uint32_t i = 0; i < numLayouts; i++)
        {
            layoutInfoSorted[std::get<2>(descriptorSetLayout[i])] = std::get<0>(descriptorSetLayout[i]);
        }
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = numLayouts;
        pipelineLayoutInfo.pSetLayouts = layoutInfoSorted;
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;
        pipelineLayoutInfo.pushConstantRangeCount = pcIndex;

        res = vkCreatePipelineLayout(static_cast<VkDevice>(ID), &pipelineLayoutInfo, nullptr, reinterpret_cast<VkPipelineLayout*>(&vrootSignature->ID));
        if(res < 0)
        {
            return creation_result<RootSignature>::err(marshall_error(res));
        }
        vrootSignature->device = make_ptr(this);
        uint32_t layout_idx = 0;
        for (uint32_t i : std::views::iota(0u, numLayouts))
        {
            if (std::get<1>(descriptorSetLayout[i]) == RootParameterType::DynamicDescriptor)
            {
                vkDestroyDescriptorSetLayout(static_cast<VkDevice>(ID), std::get<0>(descriptorSetLayout[i]), nullptr);
                continue;
            }

            pSetLayouts[layout_idx] = Ptr(new vDescriptorSetLayout);
            pSetLayouts[layout_idx]->ID = std::get<0>(descriptorSetLayout[i]);
            pSetLayouts[layout_idx]->device = make_ptr(this);
            layout_idx++;
        }
        return creation_result<RootSignature>::ok(vrootSignature);
    }
    VkCompareOp vkCompareFunc(const ComparisonFunc func)
    {
        using enum ComparisonFunc;
        switch (func)
        {
        case Never: return VK_COMPARE_OP_NEVER;
        case Less: return VK_COMPARE_OP_LESS;
        case Equal: return VK_COMPARE_OP_EQUAL;
        case LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
        case Greater: return VK_COMPARE_OP_GREATER;
        case NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
        case GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case Always: return VK_COMPARE_OP_ALWAYS;
        default: return VK_COMPARE_OP_MAX_ENUM;
        }
    }

    VkStencilOp vkStenOp(const StencilOp op)
    {
        using enum StencilOp;
        switch (op)
        {
        case Keep: return VK_STENCIL_OP_KEEP;
        case Zero: return VK_STENCIL_OP_ZERO;
        case Replace: return VK_STENCIL_OP_REPLACE;
        case IncrSat: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case DecrSat: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case Invert: return VK_STENCIL_OP_INVERT;
        case Incr: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case Decr: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        default: return VK_STENCIL_OP_MAX_ENUM;
        }
    }
    VkCullModeFlags VkCullMode(const CullMode mode)
    {
        using enum CullMode;
        switch (mode)
        {
        case None: return VK_CULL_MODE_NONE;
        case Front: return VK_CULL_MODE_FRONT_BIT;
        case Back: return VK_CULL_MODE_BACK_BIT;
        default: return VK_CULL_MODE_NONE;
        }
    }
    creation_result<PipelineStateObject> Device::CreatePipelineStateObject(const PipelineStateObjectDesc& desc)
    {
        Ptr<vPipelineStateObject> vPSO(new vPipelineStateObject);
        GFX_ASSERT(desc->numInputElements < 5);
        VkPipelineShaderStageCreateInfo ShaderpipelineInfo[5] = {};
        VkShaderModule modules[5];
        int index = 0;
        auto createShader = [&](const ShaderCode& code, const VkShaderStageFlagBits bits){
            CreationError err = CreationError::None;
            if(code.data.empty())
            {
                if(desc.shaderMode == File) err = CreateShaderModule(std::string(code.data).c_str(), ShaderpipelineInfo, bits, index, modules, ID);
                else err = CreateShaderModule(code.data.data(), code.data.size(), ShaderpipelineInfo, bits, index, modules, ID);
                index++;
            }
            return err;
        };
        auto cleanShaders = [&](CreationError err) {
            if(err == CreationError::None) return false;
            for(const auto module: std::span{modules, static_cast<size_t>(index)})
            {
                vkDestroyShaderModule(static_cast<VkDevice>(ID), module, nullptr);
            }
            return true;
        };
        CreationError err = CreationError::None;

        err = createShader(desc.VS, VK_SHADER_STAGE_VERTEX_BIT);
        if(cleanShaders(err)) return ezr::err(err);

        err = createShader(desc.PS, VK_SHADER_STAGE_FRAGMENT_BIT);
        if(cleanShaders(err)) return ezr::err(err);

        err = createShader(desc.GS, VK_SHADER_STAGE_GEOMETRY_BIT);
        if(cleanShaders(err)) return ezr::err(err);

        err = createShader(desc.HS, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
        if(cleanShaders(err)) return ezr::err(err);

        err = createShader(desc.DS, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
        if(cleanShaders(err)) return ezr::err(err);
        
        VkVertexInputAttributeDescription inputAttribDesc[5];
        VkVertexInputBindingDescription inputbindingDesc[5];
        for (uint32_t i = 0; i < desc.numInputElements ; i++)
        {
            inputAttribDesc[i].format = FormatConv(desc.inputElements[i].format);
            inputAttribDesc[i].location = desc.inputElements[i].location;
            inputAttribDesc[i].offset = desc.inputElements[i].alignedByteOffset;
            inputAttribDesc[i].binding = desc.inputElements[i].inputSlot;

        }
        for (uint32_t i = 0; i < desc.numInputBindings; i++)
        {
            inputbindingDesc[i].binding = i;
            inputbindingDesc[i].inputRate = static_cast<VkVertexInputRate>(desc.inputBindings[i].inputRate);
            inputbindingDesc[i].stride = desc.inputBindings[i].stride;
        }
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = desc.numInputBindings;
        vertexInputInfo.pVertexBindingDescriptions = inputbindingDesc; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = desc.numInputElements;
        vertexInputInfo.pVertexAttributeDescriptions = inputAttribDesc; // Optional

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = vkPrimitiveTopology(desc.rasterizerMode.topology);
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_STENCIL_REFERENCE,

        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = ARRAYSIZE(dynamicStates);
        dynamicState.pDynamicStates = dynamicStates;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VkCullMode(desc.rasterizerMode.cullMode);
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional


        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.depthTestEnable = desc.depthStencilMode.DepthEnable;
        depthStencil.depthCompareOp = vkCompareFunc(desc.depthStencilMode.DepthFunc);
        depthStencil.depthWriteEnable = desc.depthStencilMode.DepthWriteMask == DepthWriteMask::All ? 1 : 0;
        //todo depth bounds
        depthStencil.flags = 0;
        depthStencil.stencilTestEnable = desc.depthStencilMode.StencilEnable;
        depthStencil.back.compareMask = desc.depthStencilMode.StencilReadMask;
        depthStencil.back.writeMask = desc.depthStencilMode.StencilWriteMask;
        depthStencil.back.compareOp = vkCompareFunc(desc.depthStencilMode.BackFace.Stencilfunc);
        depthStencil.back.depthFailOp = vkStenOp(desc.depthStencilMode.BackFace.DepthfailOp);
        depthStencil.back.failOp = vkStenOp(desc.depthStencilMode.BackFace.failOp);
        depthStencil.back.passOp = vkStenOp(desc.depthStencilMode.BackFace.passOp);
        depthStencil.front.compareMask = desc.depthStencilMode.StencilReadMask;
        depthStencil.front.writeMask = desc.depthStencilMode.StencilWriteMask;
        depthStencil.front.compareOp = vkCompareFunc(desc.depthStencilMode.FrontFace.Stencilfunc);
        depthStencil.front.depthFailOp = vkStenOp(desc.depthStencilMode.FrontFace.DepthfailOp);
        depthStencil.front.failOp = vkStenOp(desc.depthStencilMode.FrontFace.failOp);
        depthStencil.front.passOp = vkStenOp(desc.depthStencilMode.FrontFace.passOp);
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        VkPipelineRenderingCreateInfo pipelineRendereing{};
        pipelineRendereing.colorAttachmentCount = desc.numRenderTargets;//todo
        pipelineRendereing.depthAttachmentFormat = FormatConv(desc.DSVFormat);
        VkFormat format[10];
        for (uint32_t i = 0; i < desc.numRenderTargets; i++)
        {
            format[i] = FormatConv(desc.RTVFormats[0]);
        }
        pipelineRendereing.pColorAttachmentFormats = format;
        pipelineRendereing.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.pNext = &pipelineRendereing;
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = index;
        pipelineInfo.pStages = ShaderpipelineInfo;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = static_cast<VkPipelineLayout>(desc.rootSig->ID);
        pipelineInfo.renderPass = nullptr;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional


        vPSO->device = make_ptr(this);

        if(const VkResult res = vkCreateGraphicsPipelines(static_cast<VkDevice>(ID), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, reinterpret_cast<VkPipeline*>(&vPSO->ID));
            res < 0) return creation_result<PipelineStateObject>::err(marshall_error(res));
        for(int i = 0; i < index; i++)
            vkDestroyShaderModule(static_cast<VkDevice>(ID), modules[i], nullptr);
        return creation_result<PipelineStateObject>::ok(vPSO);
    }
    creation_array_result<DescriptorSet> Device::CreateDescriptorSets(const Ptr<DescriptorHeap>& heap, std::uint32_t numDescriptorSets, Ptr<DescriptorSetLayout>* layouts)
    {
        std::vector<Ptr<DescriptorSet>> returnValues;
        VkDescriptorSetLayout vklayouts[5];
        for (uint32_t i = 0; i < numDescriptorSets; i++)
        {
            vklayouts[i] = static_cast<VkDescriptorSetLayout>(layouts[i]->ID);
        }
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = static_cast<VkDescriptorPool>(heap->ID);
        allocInfo.descriptorSetCount = numDescriptorSets;
        allocInfo.pSetLayouts = vklayouts;
        VkDescriptorSet descriptorSets[5];
        VkResult res = vkAllocateDescriptorSets(static_cast<VkDevice>(ID), &allocInfo, descriptorSets);
        if(res < 0) return creation_array_result<DescriptorSet>::err(marshall_error(res));

        for (uint32_t i = 0; i < numDescriptorSets; i++)
        {
            auto& ptr = returnValues.emplace_back(new vDescriptorSet);
            ptr.retrieve_as_forced<vDescriptorSet>()->ID = descriptorSets[i];
            ptr.retrieve_as_forced<vDescriptorSet>()->device = make_ptr(this);
            ptr.retrieve_as_forced<vDescriptorSet>()->heap = heap.transform<vDescriptorHeap>();
            ptr.retrieve_as_forced<vDescriptorSet>()->layout = layouts[i].transform<vDescriptorSetLayout>();
        }
        return creation_array_result<DescriptorSet>::ok(std::move(returnValues));
    }
    Texture* Device::WrapNativeTexture(Internal_ID id)
    {
        auto* vtex = new RHI::vTexture;
        vtex->ID = id;
        vtex->device = make_ptr(this);

        return vtex;
    }
    void Device::UpdateDescriptorSet(std::uint32_t numDescs, const DescriptorSetUpdateDesc* desc, Weak<DescriptorSet> sets) const
    {

        VkWriteDescriptorSet writes[5]{};
        VkDescriptorBufferInfo Binfo[5]{ };
        VkDescriptorImageInfo Iinfo[5]{ };
        for(uint32_t i = 0; i < numDescs; i++)
        {
            switch (desc[i].type)
            {
            case(RHI::DescriptorType::CSTexture):
            {
                Iinfo[i].imageView = static_cast<VkImageView>(desc[i].textureInfos->texture->ID);
                Iinfo[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                Iinfo[i].sampler = nullptr;
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                writes[i].pImageInfo = &Iinfo[i];
                break;
            }
            case(RHI::DescriptorType::CSBuffer):
            {
                Binfo[i].buffer = static_cast<VkBuffer>(desc[i].bufferInfos->buffer->ID);
                Binfo[i].offset = desc[i].bufferInfos->offset;
                Binfo[i].range = desc[i].bufferInfos->range;
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writes[i].pBufferInfo = &Binfo[i];
                break;
            }
            case(RHI::DescriptorType::ConstantBuffer):
            {
                Binfo[i].buffer = static_cast<VkBuffer>(desc[i].bufferInfos->buffer->ID);
                Binfo[i].offset = desc[i].bufferInfos->offset;
                Binfo[i].range = desc[i].bufferInfos->range;
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writes[i].pBufferInfo = &Binfo[i];
                break;
            }
            case(RHI::DescriptorType::SampledTexture):
            {
                Iinfo[i].imageView = static_cast<VkImageView>(desc[i].textureInfos->texture->ID);
                Iinfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                Iinfo[i].sampler = nullptr;
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                writes[i].pImageInfo = &Iinfo[i];
                break;
            }
            case(RHI::DescriptorType::StructuredBuffer):
            {
                Binfo[i].buffer = static_cast<VkBuffer>(desc[i].bufferInfos->buffer->ID);
                Binfo[i].offset = desc[i].bufferInfos->offset;
                Binfo[i].range = desc[i].bufferInfos->range;
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writes[i].pBufferInfo = &Binfo[i];
                break;
            }
            case(RHI::DescriptorType::Sampler):
            {
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                Iinfo[i].sampler = *static_cast<VkSampler*>(desc[i].samplerInfos->heapHandle.ptr);
                Iinfo[i].imageView = VK_NULL_HANDLE;
                writes[i].pImageInfo = &Iinfo[i];
            }
            default:
                break;
            }
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = static_cast<VkDescriptorSet>(sets->ID);
            writes[i].dstBinding = desc[i].binding;
            writes[i].dstArrayElement = 0;
            writes[i].descriptorCount = desc->numDescriptors;
        }
        vkUpdateDescriptorSets(static_cast<VkDevice>(ID), numDescs, writes, 0, nullptr);

    }
    auto ImageType(TextureType type) -> VkImageType
    {
        if (type == TextureType::Texture1D) return VK_IMAGE_TYPE_1D;
        if (type == TextureType::Texture2D) return VK_IMAGE_TYPE_2D;
        if (type == TextureType::Texture3D) return VK_IMAGE_TYPE_3D;
        return VK_IMAGE_TYPE_MAX_ENUM;
    }
    VkImageTiling ImageTiling(const TextureTilingMode mode)
    {
        if (mode == TextureTilingMode::Linear) return VK_IMAGE_TILING_LINEAR;
        if (mode == TextureTilingMode::Optimal) return VK_IMAGE_TILING_OPTIMAL;
        return VK_IMAGE_TILING_MAX_ENUM;
    }
    VkImageUsageFlags ImageUsage(const RHI::TextureUsage usage)
    {
        VkImageUsageFlags flags = 0;
        if ((usage & RHI::TextureUsage::SampledImage) != RHI::TextureUsage::None)
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if ((usage & RHI::TextureUsage::CopyDst) != RHI::TextureUsage::None)
            flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if ((usage & RHI::TextureUsage::ColorAttachment) != RHI::TextureUsage::None)
            flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if ((usage & RHI::TextureUsage::DepthStencilAttachment) != RHI::TextureUsage::None)
            flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if ((usage & RHI::TextureUsage::CopySrc) != RHI::TextureUsage::None)
            flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if ((usage & RHI::TextureUsage::StorageImage) != RHI::TextureUsage::None)
            flags |= VK_IMAGE_USAGE_STORAGE_BIT;

        return flags;
    }
    creation_result<DynamicDescriptor> Device::CreateDynamicDescriptor(const Ptr<DescriptorHeap>& heap, const DescriptorType type, const ShaderStage stage, const Weak<Buffer> buffer, const uint32_t offset, const uint32_t size)
    {
        if((type != DescriptorType::ConstantBufferDynamic && type != DescriptorType::StructuredBufferDynamic) || heap.ptr == nullptr)
        {
            return creation_result<DynamicDescriptor>::err(CreationError::InvalidParameters);
        }
        VkDescriptorSetLayoutBinding binding;
        binding.binding = 0;
        binding.descriptorCount = 1;
        binding.descriptorType = DescType(type);
        binding.pImmutableSamplers = nullptr;
        binding.stageFlags = VkShaderStage(stage);
        VkDescriptorSetLayoutCreateInfo info;//todo construct the set layout only once as part of the device
        info.pNext = nullptr;
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.flags = 0;
        info.pBindings = &binding;

        Ptr<vDynamicDescriptor> vdescriptor(new vDynamicDescriptor);
        VkResult res = vkCreateDescriptorSetLayout(static_cast<VkDevice>(ID), &info, nullptr, &vdescriptor->layout);

        if(res < 0) return creation_result<DynamicDescriptor>::err(marshall_error(res));

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = static_cast<VkDescriptorPool>(heap->ID);
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &vdescriptor->layout;
        res = vkAllocateDescriptorSets(static_cast<VkDevice>(ID), &allocInfo, reinterpret_cast<VkDescriptorSet*>(&vdescriptor->ID));

        if(res < 0) return creation_result<DynamicDescriptor>::err(marshall_error(res));

        vdescriptor->device = make_ptr(this);
        vdescriptor->heap = heap.transform<vDescriptorHeap>();
        VkDescriptorBufferInfo binfo{};
        binfo.buffer = static_cast<VkBuffer>(buffer->ID);
        binfo.offset = offset;
        binfo.range = size;
        VkWriteDescriptorSet write{};
        write.descriptorCount = 1;
        write.descriptorType = DescType(type);
        write.dstSet = static_cast<VkDescriptorSet>(vdescriptor->ID);
        write.pBufferInfo = &binfo;
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vkUpdateDescriptorSets(static_cast<VkDevice>(ID), 1,&write, 0, nullptr);
        return creation_result<DynamicDescriptor>::ok(vdescriptor);
    }
    creation_result<Texture> Device::CreateTexture(const TextureDesc& desc, const Ptr<Heap>& heap, const HeapProperties* props, const AutomaticAllocationInfo* automatic_info, const std::uint64_t offset, const ResourceType type)
    {
        Ptr<vTexture> vtexture(new vTexture);
        VkImageCreateInfo info{};
        info.arrayLayers = desc.type == TextureType::Texture3D ? 1 : desc.depthOrArraySize;
        info.extent.width = desc.width;
        info.extent.height = desc.height;
        info.extent.depth = desc.type == TextureType::Texture3D ? desc.depthOrArraySize : 1;
        VkImageCreateFlags flags = 0;
        if ((desc.usage & TextureUsage::CubeMap) != TextureUsage::None) flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        info.flags = flags;
        info.format = FormatConv(desc.format);
        info.imageType = ImageType(desc.type);
        info.initialLayout = static_cast<VkImageLayout>(desc.layout);
        info.mipLevels = desc.mipLevels;
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.tiling = ImageTiling(desc.mode);
        info.usage = ImageUsage(desc.usage);
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        vtexture->device = make_ptr(this);
        if (type == ResourceType::Automatic)
        {
            VmaAllocationCreateInfo allocCreateInfo{};
            allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocCreateInfo.flags = 0;
            allocCreateInfo.flags |=
                (automatic_info->access_mode == AutomaticAllocationCPUAccessMode::Random) ? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT : 0;
            allocCreateInfo.flags |=
                (automatic_info->access_mode == AutomaticAllocationCPUAccessMode::Sequential) ? VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT : 0;
            VkResult res = vmaCreateImage(reinterpret_cast<vDevice*>(this)->allocator, &info, &allocCreateInfo, reinterpret_cast<VkImage*>(&vtexture->ID), &vtexture->vma_ID, nullptr);
            if(res < 0) return creation_result<Texture>::err(marshall_error(res));
            return creation_result<Texture>::ok(vtexture);
        }
        VkResult res = vkCreateImage(static_cast<VkDevice>(ID), &info, nullptr, reinterpret_cast<VkImage*>(&vtexture->ID));
        if(res < 0) return creation_result<Texture>::err(marshall_error(res));
        if (type == ResourceType::Commited)
        {
            auto [req_size, req_align, req_mem_type] = GetTextureMemoryRequirements(desc);
            auto r = CreateHeap(HeapDesc{req_size, *props}, nullptr);
            if(r.is_err()) return r.transform([](Ptr<Heap>&)->Ptr<Texture>{return nullptr;});
            vtexture->heap = r.value();
            res = vkBindImageMemory(static_cast<VkDevice>(ID), static_cast<VkImage>(vtexture->ID), static_cast<VkDeviceMemory>(vtexture->heap->ID), 0);
            if(res < 0) return creation_result<Texture>::err(marshall_error(res));
        }
        else if (type == ResourceType::Placed)
        {
            vtexture->heap = heap;
            res = vkBindImageMemory(static_cast<VkDevice>(ID), static_cast<VkImage>(vtexture->ID), static_cast<VkDeviceMemory>(heap->ID), offset);
            if(res < 0) return creation_result<Texture>::err(marshall_error(res));
        }
        return creation_result<Texture>::ok(vtexture);
    }
    MemoryRequirements Device::GetTextureMemoryRequirements(const TextureDesc& desc)
    {
        VkImageCreateInfo info{};
        info.arrayLayers = desc.type == TextureType::Texture3D ? 1 : desc.depthOrArraySize;
        info.extent.width = desc.width;
        info.extent.height = desc.height;
        info.extent.depth = desc.type == TextureType::Texture3D ? desc.depthOrArraySize : 1;
        info.flags = 0;
        info.format = FormatConv(desc.format);
        info.imageType = ImageType(desc.type);
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.mipLevels = desc.mipLevels;
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.tiling = ImageTiling(desc.mode);
        info.usage = ImageUsage(desc.usage);
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        VkDeviceImageMemoryRequirements DeviceReq{};
        DeviceReq.sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS;
        DeviceReq.pCreateInfo = &info;
        VkMemoryRequirements2 req{};
        req.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        vkGetDeviceImageMemoryRequirementsKHR(static_cast<VkDevice>(ID), &DeviceReq, &req);
        const MemoryRequirements requirements{
            .size = req.memoryRequirements.size,
            .alignment = req.memoryRequirements.alignment,
            .memoryTypeBits = req.memoryRequirements.memoryTypeBits,
        };
        return requirements;
    }
    static VkImageViewType VkViewType(TextureViewType type)
    {
        using enum TextureViewType;
        switch (type)
        {
        case Texture1D: return VK_IMAGE_VIEW_TYPE_1D;
        case Texture2D: return VK_IMAGE_VIEW_TYPE_2D;
        case Texture3D: return VK_IMAGE_VIEW_TYPE_3D;
        case TextureCube: return VK_IMAGE_VIEW_TYPE_CUBE;
        case Texture1DArray: return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        case Texture2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case TextureCubeArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        default: return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }
    }
    creation_result<TextureView> Device::CreateTextureView(const TextureViewDesc& desc)
    {
        VkImageViewCreateInfo info{};
        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.flags = 0;
        info.format = FormatConv(desc.format);
        info.image = static_cast<VkImage>(desc.texture->ID);
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        VkImageSubresourceRange range;
        range.aspectMask = static_cast<VkImageAspectFlags>(desc.range.imageAspect);
        range.baseMipLevel =                   desc.range.IndexOrFirstMipLevel;
        range.baseArrayLayer =                 desc.range.FirstArraySlice;
        range.layerCount =                     desc.range.NumArraySlices;
        range.levelCount =                     desc.range.NumMipLevels;
        info.subresourceRange = range;
        info.viewType = VkViewType(desc.type);
        Ptr<vTextureView> vtview(new vTextureView);
        vtview->device = make_ptr(this);
        if(const VkResult res = vkCreateImageView(static_cast<VkDevice>(ID), &info, nullptr, reinterpret_cast<VkImageView*>(&vtview->ID)); res < 0)
            return creation_result<TextureView>::err(marshall_error(res));
        return creation_result<TextureView>::ok(vtview);
    }
    VkSamplerAddressMode VkAddressMode(const AddressMode mode)
    {
        using enum AddressMode;
        switch (mode)
        {
        case Border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case Clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case Wrap: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        default: return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
        }
    }
    VkFilter vkFilter(Filter mode)
    {
        return (mode == Filter::Linear) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    }
    VkSamplerMipmapMode vkMipMode(Filter mode)
    {
        return (mode == Filter::Linear) ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
    CreationError Device::CreateSampler(const SamplerDesc& desc, CPU_HANDLE heapHandle)
    {
        VkSamplerCreateInfo info;
        info.addressModeU = VkAddressMode(desc.AddressU);
        info.addressModeV = VkAddressMode(desc.AddressV);
        info.addressModeW = VkAddressMode(desc.AddressW);
        info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        info.anisotropyEnable = desc.anisotropyEnable;
        info.compareEnable = desc.compareEnable;
        info.compareOp = vkCompareFunc(desc.compareFunc);
        info.flags = 0;
        info.magFilter = vkFilter(desc.magFilter);
        info.maxAnisotropy = static_cast<float>(desc.maxAnisotropy);
        info.maxLod = desc.maxLOD == FLT_MAX ? VK_LOD_CLAMP_NONE : desc.maxLOD;
        info.minFilter = vkFilter(desc.minFilter);
        info.minLod = desc.minLOD;
        info.mipLodBias = desc.mipLODBias;
        info.mipmapMode = vkMipMode(desc.mipFilter);
        info.pNext = nullptr;
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.unnormalizedCoordinates = VK_FALSE;
        return marshall_error(vkCreateSampler(static_cast<VkDevice>(ID), &info, nullptr, static_cast<VkSampler*>(heapHandle.ptr)));
    }
    creation_result<ComputePipeline> Device::CreateComputePipeline(const ComputePipelineDesc& desc)
    {
        Ptr<vComputePipeline> vpipeline(new vComputePipeline);
        vpipeline->device = make_ptr(this);
        VkComputePipelineCreateInfo info{};
        info.layout = static_cast<VkPipelineLayout>(desc.rootSig->ID);
        VkShaderModule module;
        if (desc.mode == ShaderMode::File) CreateShaderModule(std::string(desc.CS.data).c_str(), &info.stage, VK_SHADER_STAGE_COMPUTE_BIT, 0, &module, ID);
        else CreateShaderModule(desc.CS.data.data(), desc.CS.data.size(), &info.stage, VK_SHADER_STAGE_COMPUTE_BIT, 0, &module, ID);
        info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        VkResult res = vkCreateComputePipelines(static_cast<VkDevice>(ID), VK_NULL_HANDLE, 1, &info, nullptr, reinterpret_cast<VkPipeline*>(&vpipeline->ID));

        if(res < 0) return creation_result<ComputePipeline>::err(marshall_error(res));
        vkDestroyShaderModule(static_cast<VkDevice>(ID), module, nullptr);
        return creation_result<ComputePipeline>::ok(vpipeline);
    }
    creation_result<DebugBuffer> Device::CreateDebugBuffer()
    {
        Ptr<vDebugBuffer> buff(new vDebugBuffer);
        VkAfterCrash_BufferCreateInfo info;
        info.markerCount = 1;
        const VkResult res = VkAfterCrash_CreateBuffer(reinterpret_cast<vDevice*>(this)->acDevice, &info, reinterpret_cast<VkAfterCrash_Buffer*>(&buff->ID),&buff->data);
        if(res < 0) return creation_result<DebugBuffer>::err(marshall_error(res));
        return creation_result<DebugBuffer>::ok(buff);
    }

}
