#pragma once
#include "Core.h"
#include "FormatsAndTypes.h"
#include "PhysicalDevice.h"
#include "CommandQueue.h"
#include "CommandAllocator.h"
#include "CommandList.h"
#include "DescriptorHeap.h"
#include "Texture.h"
#include "SwapChain.h"
#include "Fence.h"
#include "PipelineStateObject.h"
#include "Buffer.h"
#include "Heap.h"
#include "DebugBuffer.h"
#include "RootSignature.h"
#include "TextureView.h"
#include "result.hpp"
#include <utility>
#include <vector>

namespace RHI
{
#ifdef WIN32
	typedef HANDLE MemHandleT;
#else
	typedef int MemHandleT;
#endif // WIN32

	enum class DeviceCreateFlags
	{
		None = 0,
		ShareAutomaticMemory = 1
	};

	DEFINE_ENUM_FLAG_OPERATORS(DeviceCreateFlags);
	class RHI_API Device : public Object
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(Device);
	public:
		creation_result<CommandAllocator> CreateCommandAllocator(CommandListType type);
		creation_result<GraphicsCommandList> CreateCommandList(CommandListType type,Ptr<CommandAllocator> allocator);
		creation_result<DescriptorHeap> CreateDescriptorHeap(const DescriptorHeapDesc& desc);
		creation_result<DynamicDescriptor> CreateDynamicDescriptor(Ptr<DescriptorHeap> heap, DescriptorType type, ShaderStage stage, Weak<Buffer> buffer,uint32_t offset, uint32_t size);
		creation_result<TextureView> CreateTextureView(const TextureViewDesc& desc);
		creation_array_result<DescriptorSet> CreateDescriptorSets(Ptr<DescriptorHeap> heap, std::uint32_t numDescriptorSets, Ptr<DescriptorSetLayout>* layouts);
		void UpdateDescriptorSet(std::uint32_t numDescs, DescriptorSetUpdateDesc* desc, Weak<DescriptorSet> set);
		creation_result<Texture> CreateTexture(const TextureDesc& desc, Ptr<Heap> heap, HeapProperties* props, AutomaticAllocationInfo* automatic_info,std::uint64_t offset, ResourceType type);
		CreationError CreateRenderTargetView(Weak<Texture> texture, const RenderTargetViewDesc& desc, CPU_HANDLE heapHandle);
		CreationError CreateDepthStencilView(Weak<Texture> texture, const DepthStencilViewDesc& desc, CPU_HANDLE heapHandle);
		CreationError CreateSampler(const SamplerDesc& desc, CPU_HANDLE heapHandle);
		creation_result<PipelineStateObject> CreatePipelineStateObject(const PipelineStateObjectDesc& desc);
		creation_result<ComputePipeline> CreateComputePipeline(const ComputePipelineDesc& desc);
		creation_result<Buffer> CreateBuffer(const BufferDesc& desc, Ptr<Heap> heap, HeapProperties* props, AutomaticAllocationInfo* automatic_info, std::uint64_t offset, ResourceType type);
		MemoryReqirements GetBufferMemoryRequirements(const BufferDesc& desc);
		MemoryReqirements GetTextureMemoryRequirements(const TextureDesc& desc);
		creation_result<RootSignature> CreateRootSignature(RootSignatureDesc* desc, Ptr<DescriptorSetLayout>* pSetLayouts);
		creation_result<Heap> CreateHeap(const HeapDesc& desc, bool* usedFallback);
		creation_result<Fence> CreateFence(std::uint64_t val);
		creation_result<DebugBuffer> CreateDebugBuffer();
		std::uint32_t GetDescriptorHeapIncrementSize(DescriptorType type);
		creation_result<Texture> GetSwapChainImage(Weak<SwapChain> swapchain, std::uint32_t index);
		RESULT GetMemorySharingCapabilites();
		RESULT ExportTexture(Texture* texture, ExportOptions options, MemHandleT* handle);
		//This is not staying
		RESULT QueueWaitIdle(Weak<CommandQueue> queue);
		static creation_result<Device> FromNativeHandle(Internal_ID id, Internal_ID phys_device, Internal_ID instance,QueueFamilyIndices indices);
		RHI::Texture* WrapNativeTexture(Internal_ID id);
		~Device(){}
		static ezr::result<std::pair<Ptr<Device>, std::vector<Ptr<CommandQueue>>>, CreationError>
	       Create(RHI::PhysicalDevice* PhysicalDevice, RHI::CommandQueueDesc* commandQueueInfos, int numCommandQueues, Internal_ID instance,bool*MQ = nullptr, RHI::DeviceCreateFlags flags = RHI::DeviceCreateFlags::None);
	};
	const RESULT& vkCompareFunc();
}
