#pragma once
#include "Core.h"
#include "FormatsAndTypes.h"
#include "CommandAllocator.h"
#include "Texture.h"
#include "Barrier.h"
#include "DescriptorHeap.h"
#include "PipelineStateObject.h"
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
		RESULT Begin(CommandAllocator* allocator);
		RESULT PipelineBarrier(PipelineStage syncBefore, PipelineStage syncAfter, std::uint32_t numBufferBarriers, BufferMemoryBarrier* pbufferBarriers,std::uint32_t numImageBarriers, TextureMemoryBarrier* pImageBarriers);
		RESULT SetPipelineState(PipelineStateObject* pso);
		RESULT SetDescriptorHeap(DescriptorHeap* heap);
		RESULT BindDescriptorSet(RootSignature* rs, DescriptorSet* set, std::uint32_t rootParamIndex);
		RESULT SetScissorRects(uint32_t numRects, Area2D* rects);
		RESULT SetViewports(uint32_t numViewports, Viewport* viewports);
		RESULT CopyBufferRegion(uint32_t srcOffset, uint32_t dstOffset, uint32_t size, Buffer* srcBuffer, Buffer* dstBuffer);
		RESULT CopyBufferToImage(uint32_t srcOffset, uint32_t srcRowWidth, uint32_t srcHeight, SubResourceRange dstRange, Offset3D imgOffset, Extent3D imgSize, Buffer* buffer, Texture* texture);
		RESULT Draw(uint32_t numVertices, uint32_t numInstances, uint32_t firstVertex, uint32_t firstInstance);
		RESULT DrawIndexed(uint32_t IndexCount, uint32_t InstanceCount, uint32_t startIndexLocation, uint32_t startVertexLocation, uint32_t startInstanceLocation);
		RESULT BeginRendering(const RenderingBeginDesc* desc);
		RESULT BindVertexBuffers(uint32_t startSlot, uint32_t numBuffers, Internal_ID* buffers);
		RESULT BindIndexBuffer(Buffer* buffer, uint32_t offset);
		RESULT SetRootSignature(RootSignature* rs);
		RESULT EndRendering();
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


