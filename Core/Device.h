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
	enum class CreationError
	{
		Unknown = -1,
		None = 0,
		OutOfHostMemory = 1,
		OutOfDeviceMemory = 2,
		InvalidParameters = 3,
		FragmentedHeap = 4,
		OutOfHeapMemory = 5
	};
	template<typename T>
	using creation_result = ezr::result<T, CreationError>;
	DEFINE_ENUM_FLAG_OPERATORS(DeviceCreateFlags);
	class RHI_API Device : public Object
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(Device);
	public:
		creation_result<Ptr<CommandAllocator>> CreateCommandAllocator(CommandListType type);
		template<typename T> RESULT CreateCommandList(CommandListType type,CommandAllocator* allocator,T** pCommandList);
		creation_result<Ptr<DescriptorHeap>> CreateDescriptorHeap(DescriptorHeapDesc* desc);
		creation_result<Ptr<DynamicDescriptor>> CreateDynamicDescriptor(DescriptorHeap* heap, DescriptorType type, ShaderStage stage, RHI::Buffer* buffer,uint32_t offset, uint32_t size);
		creation_result<Ptr<TextureView>> CreateTextureView(TextureViewDesc* desc);
		RESULT CreateDescriptorSets(DescriptorHeap* heap, std::uint32_t numDescriptorSets, DescriptorSetLayout* layouts, DescriptorSet** pSets);
		void UpdateDescriptorSets(std::uint32_t numDescs, DescriptorSetUpdateDesc* desc, DescriptorSet* set);
		creation_result<Ptr<Texture>> CreateTexture(TextureDesc* desc, Texture** texture, Heap* heap, HeapProperties* props, AutomaticAllocationInfo* automatic_info,std::uint64_t offset, ResourceType type);
		CreationError CreateRenderTargetView(Texture* texture, RenderTargetViewDesc* desc, CPU_HANDLE heapHandle);
		CreationError CreateDepthStencilView(Texture* texture, DepthStencilViewDesc* desc, CPU_HANDLE heapHandle);
		CreationError CreateSampler(SamplerDesc* desc, CPU_HANDLE heapHandle);
		creation_result<Ptr<PipelineStateObject>> CreatePipelineStateObject(PipelineStateObjectDesc* desc);
		creation_result<Ptr<ComputePipeline>> CreateComputePipeline(ComputePipelineDesc* desc);
		creation_result<Ptr<Buffer>> CreateBuffer(BufferDesc* desc, Heap* heap, HeapProperties* props, AutomaticAllocationInfo* automatic_info, std::uint64_t offset, ResourceType type);
		void GetBufferMemoryRequirements(BufferDesc* desc, MemoryReqirements* requirements);
		void GetTextureMemoryRequirements(TextureDesc* desc, MemoryReqirements* requirements);
		creation_result<Ptr<RootSignature>> CreateRootSignature(RootSignatureDesc* desc, Ptr<DescriptorSetLayout>* pSetLayouts);
		creation_result<Ptr<Heap>> CreateHeap(HeapDesc* desc, bool* usedFallback);
		creation_result<Ptr<Fence>> CreateFence(Fence** fence, std::uint64_t val);
		creation_result<Ptr<DebugBuffer>> CreateDebugBuffer();
		std::uint32_t GetDescriptorHeapIncrementSize(DescriptorType type);
		RESULT GetSwapChainImage(SwapChain* swapchain, std::uint32_t index, Texture** texture);
		RESULT GetMemorySharingCapabilites();
		RESULT ExportTexture(Texture* texture, ExportOptions options, MemHandleT* handle);
		//This is not staying
		RESULT QueueWaitIdle(CommandQueue* queue);
		static RHI::Device* FromNativeHandle(Internal_ID id, Internal_ID phys_device, Internal_ID instance,QueueFamilyIndices indices);
		RHI::Texture* WrapNativeTexture(Internal_ID id);
		~Device(){}
	};
	template<> RESULT Device::CreateCommandList<GraphicsCommandList>(CommandListType type, CommandAllocator* allocator, GraphicsCommandList** ppCommandList);
	const RESULT& vkCompareFunc();
}

extern "C"
{
	RESULT RHI_API RHICreateDevice(RHI::PhysicalDevice* PhysicalDevice, RHI::CommandQueueDesc* commandQueueInfos, int numCommandQueues, RHI::CommandQueue** commandQueues, Internal_ID instance, RHI::Device** device,bool*MQ = nullptr, RHI::DeviceCreateFlags flags = RHI::DeviceCreateFlags::None);
}
