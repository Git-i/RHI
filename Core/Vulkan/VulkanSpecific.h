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
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>
namespace RHI
{


    CreationError marshall_error(VkResult r);
    std::pair<QueueFamilyIndices, std::vector<uint32_t>> findQueueFamilyIndices(Weak<PhysicalDevice> device, Weak<Surface> surface = nullptr);
    class vInstance final : public Instance
    {
    public:
        inline static Weak<vInstance> current = nullptr;
        std::function<void(LogLevel, std::string_view)> logCallback;
        VkDebugUtilsMessengerEXT messenger;
        ~vInstance() override
        {
            vkDestroyDebugUtilsMessengerEXT(static_cast<VkInstance>(ID), messenger, nullptr);
            vkDestroyInstance(static_cast<VkInstance>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_INSTANCE;
        }
    };
    class vDevice final : public Device
    {
    public:
        ~vDevice() override
        {
            vmaDestroyAllocator(allocator);
            VkAfterCrash_DestroyDevice(acDevice);
            vkDestroyDevice(static_cast<VkDevice>(ID), nullptr);
            vInstance::current->Release();
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
        //to keep track of all refs to it, should be removed at release
        std::unordered_map<DeviceChild*, std::string> objects;
    };

    VkFormat FormatConv(RHI::Format format);
    VkPrimitiveTopology vkPrimitiveTopology(PrimitiveTopology topology);
    class vResource
    {
    public:
        struct PlacedData
        {
            std::uint64_t offset;
            std::uint64_t size;
            Ptr<Heap> heap;
        };
        std::variant<PlacedData, VmaAllocation> allocation;
        [[nodiscard]] bool is_auto() const { return std::holds_alternative<VmaAllocation>(allocation);}
        [[nodiscard]] VmaAllocation& vma_allocation() { return std::get<1>(allocation);}
        [[nodiscard]] PlacedData& placed_data() { return std::get<0>(allocation);}

        uint8_t* mapped_data = nullptr;
        std::atomic<uint32_t> map_ref = 0;
    };
    class vBuffer final : public Buffer, public vResource
    {
    public:
        ~vBuffer() override
        {
            if(is_auto()) vmaDestroyBuffer(device.retrieve_as_forced<vDevice>()->allocator, static_cast<VkBuffer>(ID), vma_allocation());
            else vkDestroyBuffer( static_cast<VkDevice>(device->ID), static_cast<VkBuffer>(ID), nullptr);
            ID = nullptr;
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
            if (!this->m_pools.empty())
            {
                vkFreeCommandBuffers(static_cast<VkDevice>(device->ID), static_cast<VkCommandPool>(ID), m_pools.size(), reinterpret_cast<VkCommandBuffer*>(m_pools.data()));
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
    class vSurface final : public Surface
    {
    public:
        ~vSurface() override
        {
            vkDestroySurfaceKHR(static_cast<VkInstance>(instance->ID), static_cast<VkSurfaceKHR>(ID), nullptr);
        }
        Ptr<Instance> instance;
    };
    class vDebugBuffer final : public DebugBuffer
    {
    public:
        ~vDebugBuffer() override
        {
            VkAfterCrash_DestroyBuffer(static_cast<VkAfterCrash_Buffer>(ID));
        }
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
            if(is_auto()) vmaDestroyImage(device.retrieve_as_forced<vDevice>()->allocator, static_cast<VkImage>(ID), vma_allocation());
            else vkDestroyImage(static_cast<VkDevice>(device->ID), static_cast<VkImage>(Texture::ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_IMAGE;
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
            for(auto& texture : textures)
            {
                texture = nullptr;
            }
            vkDestroyFence(static_cast<VkDevice>(device->ID), imageAcquired, nullptr);
            vkDestroySwapchainKHR(static_cast<VkDevice>(device->ID), static_cast<VkSwapchainKHR>(ID), nullptr);
        }
        int32_t GetType() override
        {
            return VK_OBJECT_TYPE_SWAPCHAIN_KHR;
        }
    public:
        std::vector<Ptr<Texture>> textures;
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
