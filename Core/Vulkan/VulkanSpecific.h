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
    
    

    std::pair<QueueFamilyIndices, std::vector<uint32_t>> findQueueFamilyIndices(RHI::PhysicalDevice* device, RHI::Surface surface = RHI::Surface());

    class vDevice : public Device
    {
    public:
        virtual void Destroy() override
        {
            delete Object::refCnt;
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
        Internal_ID heap;
        VmaAllocation vma_ID = 0;//for automatic resources
    };
    class vBuffer : public Buffer, public vResource
    {
    public:
        virtual void Destroy() override
        {
            delete Object::refCnt;
            if(vma_ID) vmaDestroyBuffer(((vDevice*)device)->allocator, (VkBuffer)ID, vma_ID);
            else vkDestroyBuffer( (VkDevice) ((vDevice*)device)->ID, (VkBuffer)ID, nullptr);
            vma_ID = nullptr; ID = nullptr;
            ((vDevice*)device)->Release();
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_BUFFER;
        }
    };
    
    class vCommandAllocator : public CommandAllocator
    {
    public:
        virtual void Destroy() override
        {
            delete Object::refCnt;
            if (((vCommandAllocator*)this)->m_pools.size())
            {
                vkFreeCommandBuffers((VkDevice)((vDevice*)device)->ID, (VkCommandPool)CommandAllocator::ID, m_pools.size(), (VkCommandBuffer*)m_pools.data());
            }
            vkDestroyCommandPool((VkDevice)((vDevice*)device)->ID, (VkCommandPool)CommandAllocator::ID, nullptr);
            ((vDevice*)device)->Release();
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
    
    class vDynamicDescriptor : public DynamicDescriptor
    {

    };
    class vDescriptorHeap : public DescriptorHeap
    {
    public:
        virtual void Destroy() override
        {
            delete Object::refCnt;
            vkDestroyDescriptorPool((VkDevice)((vDevice*)device)->ID, (VkDescriptorPool)DescriptorHeap::ID, nullptr);
            ((vDevice*)device)->Release();
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_DESCRIPTOR_POOL;
        }
    };
    class vFence : public Fence
    {
    public:
        virtual void Destroy() override
        {
            delete Object::refCnt;
            vkDestroySemaphore((VkDevice)((vDevice*)device)->ID, (VkSemaphore)Fence::ID, nullptr);
            ((vDevice*)device)->Release();
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_SEMAPHORE;
        }
    };
    class vHeap : public Heap
    {
    public:
        virtual void Destroy() override
        {
            delete Object::refCnt;
            vkFreeMemory((VkDevice)((vDevice*)device)->ID, (VkDeviceMemory)Heap::ID, nullptr);
            ((vDevice*)device)->Release();
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_DEVICE_MEMORY;
        }
    };
    class vDescriptorSetLayout : public DescriptorSetLayout
    {
    public:
        virtual void Destroy() override
        {
            delete Object::refCnt;
            vkDestroyDescriptorSetLayout((VkDevice)((vDevice*)device)->ID, (VkDescriptorSetLayout)DescriptorSetLayout::ID, nullptr);
            ((vDevice*)device)->Release();
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
        }
    };
    class vRootSignature : public RootSignature
    {
    public:
        virtual void Destroy() override
        {
            delete Object::refCnt;
            vkDestroyPipelineLayout((VkDevice)((vDevice*)device)->ID, (VkPipelineLayout)RootSignature::ID, nullptr);
            ((vDevice*)device)->Release();
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
        void Destroy() override
        {
            if (auto pos = std::find(allocator->m_pools.begin(), allocator->m_pools.end(), ID); pos != allocator->m_pools.end())
            {
                allocator->m_pools.erase(pos);
            }
            vkFreeCommandBuffers((VkDevice)((vDevice*)device)->ID, (VkCommandPool)allocator->ID, 1, (VkCommandBuffer*)&ID);
            ((vDevice*)device)->Release();
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_COMMAND_BUFFER;
        }
        vCommandAllocator* allocator;
        vRootSignature* currentRS = nullptr;
    };
    class vPipelineStateObject : public PipelineStateObject
    {
    public:
        virtual void Destroy() override
        {
            delete Object::refCnt;
            vkDestroyPipeline((VkDevice)((vDevice*)device)->ID, (VkPipeline)PipelineStateObject::ID, nullptr);
            ((vDevice*)device)->Release();
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_PIPELINE;
        }
    };
    class vComputePipeline : public ComputePipeline
    {
    public:
        virtual void Destroy() override
        {
            delete Object::refCnt;
            vkDestroyPipeline((VkDevice)((vDevice*)device)->ID, (VkPipeline)ComputePipeline::ID, nullptr);
            ((vDevice*)device)->Release();
        }
        virtual int32_t GetType() override
        {
            return VK_OBJECT_TYPE_PIPELINE;
        }
    };
    class vDescriptorSet : public DescriptorSet
    {
    };
    class vTexture : public Texture, public vResource
    {
    public:
        virtual void Destroy() override
        {
            delete Object::refCnt;
            if(vma_ID) vmaDestroyImage(((vDevice*)device)->allocator, (VkImage)ID, vma_ID);
            else vkDestroyImage((VkDevice)((vDevice*)device)->ID, (VkImage)Texture::ID, nullptr);
            ((vDevice*)device)->Release();
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
        virtual void Destroy() override
        {
            delete Object::refCnt;
            vkDestroyInstance((VkInstance)Instance::ID, nullptr);
            ((vDevice*)device)->Release();
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
        virtual void Destroy() override
        {
            delete Object::refCnt;
            vkDestroySwapchainKHR((VkDevice)((vDevice*)device)->ID, (VkSwapchainKHR)SwapChain::ID, nullptr);
            ((vDevice*)device)->Release();
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
        virtual void Destroy() override
        {
            delete Object::refCnt;
            delete ((SpvReflectShaderModule*)Object::ID);
        }
    };
}
