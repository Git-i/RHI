#pragma once
#include "Core.h"
#include "FormatsAndTypes.h"
#include "CommandAllocator.h"
#include "Texture.h"
#include "Barrier.h"
#include "DescriptorHeap.h"
#include "DebugBuffer.h"
#include "PipelineStateObject.h"
#include <span>
namespace RHI
{
	struct RenderingBeginDesc;
	class RHI_API CommandList : public Object
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(CommandList);
		friend class Device;
	};
	struct Extent3D
	{
		uint32_t width, height, depth;
	};
	struct Offset3D
	{
		int32_t width, height, depth;
	};
	class RHI_API GraphicsCommandList : public CommandList
	{
	public:
		//CommandAllocator Passed must be the one used to create the command list.
		void CopyTextureRegion(SubResourceRange srcRange, SubResourceRange dstRange, Offset3D srcOffset, Offset3D dstOffset, Extent3D extent,Weak<Texture> src,Weak<Texture> dst);
		RESULT Begin(Ptr<CommandAllocator> allocator);
		void PipelineBarrier(PipelineStage syncBefore, PipelineStage syncAfter, std::span<BufferMemoryBarrier> bufferBarriers, std::span<TextureMemoryBarrier> textureBarriers);
		void ReleaseBarrier(PipelineStage syncBefore, PipelineStage syncAfter, std::span<BufferMemoryBarrier> bufferBarriers, std::span<TextureMemoryBarrier> textureBarriers);
		void SetPipelineState(Weak<PipelineStateObject> pso);
		void SetComputePipeline(Weak<ComputePipeline> cp);
		void BindComputeDescriptorSet(Weak<DescriptorSet> set, std::uint32_t setIndex);
		void BindComputeDynamicDescriptor(Weak<DynamicDescriptor> set, std::uint32_t setIndex, std::uint32_t offset);
		void SetDescriptorHeap(Weak<DescriptorHeap> heap);
		void BindDescriptorSet(Weak<DescriptorSet> set, std::uint32_t setIndex);
		void BindDynamicDescriptor(Weak<DynamicDescriptor> set, std::uint32_t setIndex, std::uint32_t offset);
		void SetScissorRects(uint32_t numRects, Area2D* rects);
		void SetViewports(uint32_t numViewports, Viewport* viewports);
		void CopyBufferRegion(uint32_t srcOffset, uint32_t dstOffset, uint32_t size, Weak<Buffer> srcBuffer, Weak<Buffer> dstBuffer);
		void CopyBufferToImage(uint32_t srcOffset, SubResourceRange dstRange, Offset3D imgOffset, Extent3D imgSize, Weak<Buffer> buffer, Weak<Texture> texture);
		void Draw(uint32_t numVertices, uint32_t numInstances, uint32_t firstVertex, uint32_t firstInstance);
		void DrawIndexed(uint32_t IndexCount, uint32_t InstanceCount, uint32_t startIndexLocation, uint32_t startVertexLocation, uint32_t startInstanceLocation);
		void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
		void BeginRendering(const RenderingBeginDesc& desc);
		void BindVertexBuffers(uint32_t startSlot, uint32_t numBuffers,const Internal_ID* buffers);
		void BindIndexBuffer(Weak<Buffer> buffer, uint32_t offset);
		void SetRootSignature(Weak<RootSignature> rs);
		void EndRendering();
		void PushConstants(uint32_t bindingIndex, uint32_t numConstants,const void* constants, uint32_t offsetIn32BitSteps);
		void PushConstant(uint32_t bindingIndex, uint32_t constant, uint32_t offsetIn32BitSteps);
		//todo (properly)
		void BlitTexture(Weak<Texture> src, Weak<Texture> dst, Extent3D srcSize, Offset3D srcOffset, Extent3D dstSize, Offset3D dstOffset, SubResourceRange srcRange, SubResourceRange dstRange);
		void MarkBuffer(Weak<Buffer> buffer, uint32_t offset, uint32_t val);
		void MarkBuffer(Weak<DebugBuffer> buffer, uint32_t val);
		//RESULT SetRenderTargetView(CPU_HANDLE rtv);
		RESULT End();
	};

	struct RHI_API RenderingAttachmentDesc
	{
		CPU_HANDLE ImageView;
		LoadOp loadOp;
		StoreOp storeOp;
		Color clearColor;
	};


	struct RHI_API RenderingBeginDesc
	{
		const RenderingAttachmentDesc* pColorAttachments;
		const RenderingAttachmentDesc* pDepthStencilAttachment;
		Area2D renderingArea;
		uint32_t numColorAttachments;
	};

}
