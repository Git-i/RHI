#pragma once
#include "../FormatsAndTypes.h"
#include "../PhysicalDevice.h"
#include "../Surface.h"
#include "volk.h"
#include <string_view>
#include <vector>
#include "../Object.h"
#include "../Buffer.h"
#include "../CommandAllocator.h"
#include "../CommandList.h"
#include "../CommandQueue.h"
#include "../Device.h"
#include "../DescriptorHeap.h"
#include "../Fence.h"
#include "../Instance.h"
#include "../ShaderReflect.h"
#include "../TextureView.h"
#include "vk_mem_alloc.h"
#include "spirv_reflect.h"
#include "../DebugBuffer.h"
#include "VulkanAfterCrash.h"
#include <algorithm>//for std::find
#include <unordered_map>
#include <vulkan/vulkan_core.h>
namespace RHI
{


    CreationError marshall_error(VkResult r);
    std::pair<QueueFamilyIndices, std::vector<uint32_t>> findQueueFamilyIndices(RHI::PhysicalDevice* device, RHI::Surface surface = RHI::Surface());

    class vDevice final : public Device
    {
    public:
        ~vDevice() override
        {
            vmaDestroyAllocator(allocator);
            VkAfterCrash_DestroyDevice(acDevice);
            vkDestroyDevice(static_cast<VkDevice>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_DEVICE;
        }
    public:
        std::vector<HeapProperties> HeapProps;
        VmaAllocator allocator;
        VkAfterCrash_Device acDevice;
        std::uint32_t DefaultHeapIndex = UINT32_MAX;
        std::uint32_t UploadHeapIndex = UINT32_MAX;
        std::uint32_t ReadBackHeapIndex = UINT32_MAX;
        QueueFamilyIndices indices;
    };

    VkFormat FormatConv(RHI::Format format);
    VkPrimitiveTopology vkPrimitiveTopology(PrimitiveTopology topology);
    class vResource
    {
    public:
        std::uint64_t offset;
        std::uint64_t size;
        Ptr<Heap> heap;
        VmaAllocation vma_ID = nullptr;//for automatic resources
    };
    class vBuffer final : public Buffer, public vResource
    {
    public:
        ~vBuffer() override
        {
            if(vma_ID) vmaDestroyBuffer(device.retrieve_as_forced<vDevice>()->allocator, static_cast<VkBuffer>(ID), vma_ID);
            else vkDestroyBuffer( static_cast<VkDevice>((device.retrieve_as_forced<vDevice>())->ID), static_cast<VkBuffer>(ID), nullptr);
            vma_ID = nullptr; ID = nullptr;
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_BUFFER;
        }
    };

    class vCommandAllocator final: public CommandAllocator
    {
    public:
        ~vCommandAllocator() override
        {
            if (((vCommandAllocator*)this)->m_pools.empty())
            {
                vkFreeCommandBuffers(static_cast<VkDevice>((device.retrieve_as_forced<vDevice>())->ID), static_cast<VkCommandPool>(ID), m_pools.size(), reinterpret_cast<VkCommandBuffer*>(m_pools.data()));
            }
            vkDestroyCommandPool(static_cast<VkDevice>(device->ID), static_cast<VkCommandPool>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_COMMAND_POOL;
        }
        std::vector<Internal_ID> m_pools;
    };

    class vTextureView final : public TextureView
    {
    public:
        ~vTextureView() override
        {
            vkDestroyImageView(static_cast<VkDevice>(device->ID), static_cast<VkImageView>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_IMAGE_VIEW;
        }
    };
    class vDebugBuffer final : public DebugBuffer
    {
    public:
        uint32_t* data;

    };
    class vCommandQueue final : public CommandQueue
    {
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_QUEUE;
        }
    };

    class vDescriptorHeap final : public DescriptorHeap
    {
    public:
        ~vDescriptorHeap() override
        {
            if(type == DescriptorType::RTV)
            {
                auto begin = static_cast<VkImageView*>(ID);
                for (const auto& elem : std::ranges::subrange(begin, begin + num_descriptors))
                {
                    if(elem) vkDestroyImageView(static_cast<VkDevice>(device->ID), elem, nullptr);
                }
                delete[] static_cast<VkImageView*>(ID);
            }
            else if(type == DescriptorType::Sampler)
            {
                auto begin = static_cast<VkSampler*>(ID);
                for (const auto& elem : std::ranges::subrange(begin, begin + num_descriptors))
                {
                    if(elem) vkDestroySampler(static_cast<VkDevice>(device->ID), elem, nullptr);
                }
                delete[] static_cast<VkSampler*>(ID);
            }
            else vkDestroyDescriptorPool(static_cast<VkDevice>(device->ID), static_cast<VkDescriptorPool>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_DESCRIPTOR_POOL;
        }
        uint32_t num_descriptors;
        DescriptorType type;
    };
    class vDynamicDescriptor final: public DynamicDescriptor
    {
    public:
    ~vDynamicDescriptor() override
    {
        if(layout) vkDestroyDescriptorSetLayout(static_cast<VkDevice>(device->ID), layout, nullptr);
    };
    VkDescriptorSetLayout layout = nullptr;
    Ptr<vDescriptorHeap> heap = nullptr;
    };
    class vFence final : public Fence
    {
    public:
        ~vFence() override
        {
            vkDestroySemaphore(static_cast<VkDevice>(device->ID), static_cast<VkSemaphore>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_SEMAPHORE;
        }
    };
    class vHeap final : public Heap
    {
    public:
        ~vHeap() override
        {
            vkFreeMemory(static_cast<VkDevice>(device->ID), static_cast<VkDeviceMemory>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_DEVICE_MEMORY;
        }
    };
    class vDescriptorSetLayout final : public DescriptorSetLayout
    {
    public:
        ~vDescriptorSetLayout() override
        {
            vkDestroyDescriptorSetLayout(static_cast<VkDevice>(device->ID), static_cast<VkDescriptorSetLayout>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
        }
    };
    class vRootSignature final : public RootSignature
    {
    public:
        ~vRootSignature() override
        {
            vkDestroyPipelineLayout(static_cast<VkDevice>(device->ID), static_cast<VkPipelineLayout>(ID), nullptr);

        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_PIPELINE_LAYOUT;
        }
        std::unordered_map<uint32_t, VkShaderStageFlags> pcBindingToStage;
    };
    class vGraphicsCommandList final : public GraphicsCommandList
    {
    public:
        ~vGraphicsCommandList() override
        {
            if (const auto pos = std::ranges::find(allocator->m_pools, ID); pos != allocator->m_pools.end())
            {
                allocator->m_pools.erase(pos);
            }
            vkFreeCommandBuffers(static_cast<VkDevice>(device->ID), static_cast<VkCommandPool>(allocator->ID), 1, reinterpret_cast<VkCommandBuffer*>(&ID));
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_COMMAND_BUFFER;
        }
        //state maintenance
        Ptr<vCommandAllocator> allocator;
        Weak<vRootSignature> currentRS = nullptr;
    };
    class vPipelineStateObject final : public PipelineStateObject
    {
    public:
        ~vPipelineStateObject() override
        {
            vkDestroyPipeline(static_cast<VkDevice>(device->ID), static_cast<VkPipeline>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_PIPELINE;
        }
    };
    class vComputePipeline final : public ComputePipeline
    {
    public:
        ~vComputePipeline() override
        {
            vkDestroyPipeline(static_cast<VkDevice>(device->ID), static_cast<VkPipeline>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_PIPELINE;
        }
    };
    class vDescriptorSet final : public DescriptorSet
    {
    public:
        Ptr<vDescriptorSetLayout> layout;
        Ptr<vDescriptorHeap> heap;
    };
    class vTexture final : public Texture, public vResource
    {
    public:
        ~vTexture() override
        {
            if(vma_ID) vmaDestroyImage(device.retrieve_as_forced<vDevice>()->allocator, static_cast<VkImage>(ID), vma_ID);
            else vkDestroyImage(static_cast<VkDevice>(device->ID), static_cast<VkImage>(Texture::ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_IMAGE;
        }
    };
    class vInstance final : public Instance
    {
    public:
        static vInstance* current;
        std::function<void(LogLevel, std::string_view)> logCallback;
        VkDebugUtilsMessengerEXT messenger;
        ~vInstance() override
        {
            vkDestroyInstance(static_cast<VkInstance>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_INSTANCE;
        }
    };
    class vPhysicalDevice : public PhysicalDevice
    {
    };
    class vSwapChain final : public SwapChain
    {
    public:
        ~vSwapChain() override
        {
            vkDestroySwapchainKHR(static_cast<VkDevice>(device->ID), static_cast<VkSwapchainKHR>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_SWAPCHAIN_KHR;
        }
    public:
        uint32_t imgIndex;
        VkFence imageAcquired;
        VkQueue PresentQueue_ID;
    };
    class vShaderReflection final : public ShaderReflection
    {
    public:
        ~vShaderReflection() override
        {
            delete static_cast<SpvReflectShaderModule*>(ID);
        }
    };
    void log(LogLevel lvl, std::string_view msg);
}
