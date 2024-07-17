#pragma once
#include "../FormatsAndTypes.h"
#include "../PhysicalDevice.h"
#include "../Surface.h"
#include "volk.h"
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

    class vDevice : public Device
    {
    public:
        virtual ~vDevice() override
        {
            delete Object::refCnt;
            vmaDestroyAllocator(allocator);
            VkAfterCrash_DestroyDevice(acDevice);
            vkDestroyDevice((VkDevice)ID, nullptr);
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_DEVICE;
        }
    public:
        std::vector<HeapProperties> HeapProps;
        VmaAllocator allocator;
        VkAfterCrash_Device acDevice;
        std::uint32_t DefaultHeapIndex = UINT32_MAX;
        std::uint32_t UploadHeapIndex = UINT32_MAX;
        std::uint32_t ReadbackHeapIndex = UINT32_MAX;
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
        VmaAllocation vma_ID = 0;//for automatic resources
    };
    class vBuffer : public Buffer, public vResource
    {
    public:
        virtual ~vBuffer() override
        {
            delete Object::refCnt;
            if(vma_ID) vmaDestroyBuffer(device.retrieve_as_forced<vDevice>()->allocator, (VkBuffer)ID, vma_ID);
            else vkDestroyBuffer( (VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkBuffer)ID, nullptr);
            vma_ID = nullptr; ID = nullptr;
            if(heap.retrieve().IsValid()) heap->Release();
            (device.retrieve_as_forced<vDevice>())->Release();
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_BUFFER;
        }
    };

    class vCommandAllocator final: public CommandAllocator
    {
    public:
        ~vCommandAllocator() override
        {
            delete Object::refCnt;
            if (((vCommandAllocator*)this)->m_pools.size())
            {
                vkFreeCommandBuffers((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkCommandPool)CommandAllocator::ID, m_pools.size(), (VkCommandBuffer*)m_pools.data());
            }
            vkDestroyCommandPool((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkCommandPool)CommandAllocator::ID, nullptr);
            (device.retrieve_as_forced<vDevice>())->Release();
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_COMMAND_POOL;
        }
        std::vector<Internal_ID> m_pools;
    };

    class vTextureView : public TextureView
    {
        public:
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_IMAGE_VIEW;
        }
    };
    class vDebugBuffer : public DebugBuffer
    {
    public:
        uint32_t* data;

    };
    class vCommandQueue : public CommandQueue
    {
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_QUEUE;
        }
    };

    class vDescriptorHeap : public DescriptorHeap
    {
    public:
        virtual ~vDescriptorHeap() override
        {
            delete Object::refCnt;
            vkDestroyDescriptorPool((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkDescriptorPool)DescriptorHeap::ID, nullptr);
            (device.retrieve_as_forced<vDevice>())->Release();
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_DESCRIPTOR_POOL;
        }
    };
    class vDynamicDescriptor final: public DynamicDescriptor
    {
    public:
    ~vDynamicDescriptor() override
    {
        if(layout) vkDestroyDescriptorSetLayout((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, layout, nullptr);
    };
    VkDescriptorSetLayout layout = nullptr;
    Ptr<vDescriptorHeap> heap = nullptr;
    };
    class vFence : public Fence
    {
    public:
        virtual ~vFence() override
        {
            delete Object::refCnt;
            vkDestroySemaphore((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkSemaphore)Fence::ID, nullptr);
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_SEMAPHORE;
        }
    };
    class vHeap : public Heap
    {
    public:
        virtual ~vHeap() override
        {
            delete Object::refCnt;
            vkFreeMemory((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkDeviceMemory)Heap::ID, nullptr);
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_DEVICE_MEMORY;
        }
    };
    class vDescriptorSetLayout : public DescriptorSetLayout
    {
    public:
        virtual ~vDescriptorSetLayout() override
        {
            delete Object::refCnt;
            vkDestroyDescriptorSetLayout((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkDescriptorSetLayout)DescriptorSetLayout::ID, nullptr);
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
        }
    };
    class vRootSignature : public RootSignature
    {
    public:
        virtual ~vRootSignature() override
        {
            delete Object::refCnt;
            vkDestroyPipelineLayout((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkPipelineLayout)RootSignature::ID, nullptr);

        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_PIPELINE_LAYOUT;
        }
        std::unordered_map<uint32_t, VkShaderStageFlags> pcBindingToStage;
    };
    class vGraphicsCommandList : public GraphicsCommandList
    {
    public:
        ~vGraphicsCommandList() override
        {
            if (auto pos = std::find(allocator->m_pools.begin(), allocator->m_pools.end(), ID); pos != allocator->m_pools.end())
            {
                allocator->m_pools.erase(pos);
            }
            vkFreeCommandBuffers((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkCommandPool)allocator->ID, 1, (VkCommandBuffer*)&ID);
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_COMMAND_BUFFER;
        }
        //state maintenance
        Ptr<vCommandAllocator> allocator;
        Weak<vRootSignature> currentRS = nullptr;
    };
    class vPipelineStateObject : public PipelineStateObject
    {
    public:
        virtual ~vPipelineStateObject() override
        {
            delete Object::refCnt;
            vkDestroyPipeline((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkPipeline)PipelineStateObject::ID, nullptr);
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_PIPELINE;
        }
    };
    class vComputePipeline : public ComputePipeline
    {
    public:
        virtual ~vComputePipeline() override
        {
            delete Object::refCnt;
            vkDestroyPipeline((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkPipeline)ComputePipeline::ID, nullptr);
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_PIPELINE;
        }
    };
    class vDescriptorSet : public DescriptorSet
    {
    public:
        Ptr<vDescriptorSetLayout> layout;
        Ptr<vDescriptorHeap> heap;
    };
    class vTexture : public Texture, public vResource
    {
    public:
        virtual ~vTexture() override
        {
            delete Object::refCnt;
            if(vma_ID) vmaDestroyImage((device.retrieve_as_forced<vDevice>())->allocator, (VkImage)ID, vma_ID);
            else vkDestroyImage((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkImage)Texture::ID, nullptr);
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_IMAGE;
        }
    };
    class vInstance : public Instance
    {
    public:
        VkDebugUtilsMessengerEXT messanger;
        virtual ~vInstance() override
        {
            delete Object::refCnt;
            vkDestroyInstance((VkInstance)Instance::ID, nullptr);
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_INSTANCE;
        }
    };
    class vPhysicalDevice : public PhysicalDevice
    {
    };
    class vSwapChain : public SwapChain
    {
    public:
        virtual ~vSwapChain() override
        {
            delete Object::refCnt;
            vkDestroySwapchainKHR((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkSwapchainKHR)SwapChain::ID, nullptr);
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_SWAPCHAIN_KHR;
        }
    public:
        std::vector<VkSemaphore> present_semaphore;
        VkQueue PresentQueue_ID;
    };
    class vShaderReflection : public ShaderReflection
    {
    public:
        virtual ~vShaderReflection() override
        {
            delete Object::refCnt;
            delete ((SpvReflectShaderModule*)Object::ID);
        }
    };
}
