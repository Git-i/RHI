#include "CommandAllocator.h"
#include "FormatsAndTypes.h"
#include "pch.h"
#include "../CommandList.h"
#include "volk.h"
#include "VulkanSpecific.h"
#include <type_traits>
namespace RHI
{
    static VkAttachmentLoadOp vloadOp(LoadOp op)
    {
        switch (op)
        {
        case(LoadOp::Load): return VK_ATTACHMENT_LOAD_OP_LOAD;
        case(LoadOp::Clear): return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case(LoadOp::DontCare): return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        default: return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
        }
    }
    static VkAttachmentStoreOp vStoreOp(StoreOp op)
    {
        switch (op)
        {
        case(StoreOp::Store): return VK_ATTACHMENT_STORE_OP_STORE;
        case(StoreOp::DontCare): return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        default: return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
        }
    }
    RESULT GraphicsCommandList::Begin(Ptr<CommandAllocator> allocator)
    {
        VkCommandBufferBeginInfo bufferBeginInfo = {};
        bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        VkCommandBufferAllocateInfo Info = {};
        Info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        Info.commandPool = (VkCommandPool)allocator->ID;
        Info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        Info.commandBufferCount = 1;
        //vkFreeCommandBuffers((VkDevice)Device_ID, (VkCommandPool)(((vGraphicsCommandList*)this)->m_allocator), 1, (VkCommandBuffer*)&ID);
        //vkResetCommandBuffer((VkCommandBuffer)ID, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        vkAllocateCommandBuffers((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, &Info, (VkCommandBuffer*)&ID);
        if(name) SetName(name);
        ((vGraphicsCommandList*)this)->allocator = allocator.transform<vCommandAllocator>();
        allocator.retrieve_as_forced<vCommandAllocator>()->m_pools.push_back(ID);
        return vkBeginCommandBuffer((VkCommandBuffer)ID, &bufferBeginInfo);
    }
    uint32_t QueueFamilyInd(Weak<vDevice> device, QueueFamily fam)
    {
        switch (fam)
        {
        case RHI::QueueFamily::Graphics: return device->indices.graphicsIndex;
            break;
        case RHI::QueueFamily::Compute: return device->indices.computeIndex;
            break;
        case RHI::QueueFamily::Copy: return device->indices.copyIndex;
            break;
        case RHI::QueueFamily::Ignored: return VK_QUEUE_FAMILY_IGNORED;
            break;
        default: return VK_QUEUE_FAMILY_IGNORED;
            break;
        }
    }
    void GraphicsCommandList::PipelineBarrier(PipelineStage syncBefore, PipelineStage syncAfter, std::span<BufferMemoryBarrier> bufferBarriers, std::span<TextureMemoryBarrier> textureBarriers)
    {
        VkBufferMemoryBarrier BufferBarr[10]{};
        VkImageMemoryBarrier ImageBarr[100]{};
        for (uint32_t i = 0; i < bufferBarriers.size(); i++)
        {
            auto& barrier = bufferBarriers[i];
            BufferBarr[i].buffer = (VkBuffer)barrier.buffer->ID;
            BufferBarr[i].srcAccessMask = (VkAccessFlags)barrier.AccessFlagsBefore;
            BufferBarr[i].dstAccessMask = (VkAccessFlags)barrier.AccessFlagsAfter;
            BufferBarr[i].srcQueueFamilyIndex = QueueFamilyInd(device.retrieve_as_forced<vDevice>(), barrier.previousQueue);
            BufferBarr[i].dstQueueFamilyIndex = QueueFamilyInd(device.retrieve_as_forced<vDevice>(), barrier.nextQueue);
            if (BufferBarr[i].srcQueueFamilyIndex == BufferBarr[i].dstQueueFamilyIndex)
                BufferBarr[i].srcQueueFamilyIndex = BufferBarr[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            BufferBarr[i].offset = barrier.offset;
            BufferBarr[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            BufferBarr[i].size = barrier.size;
        }
        for (uint32_t i = 0; i < textureBarriers.size(); i++)
        {
            auto& barrier = textureBarriers[i];
            ImageBarr[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            ImageBarr[i].image = (VkImage)barrier.texture->ID;
            ImageBarr[i].newLayout = (VkImageLayout)barrier.newLayout;
            ImageBarr[i].oldLayout = (VkImageLayout)barrier.oldLayout;
            ImageBarr[i].srcAccessMask = (VkAccessFlags)barrier.AccessFlagsBefore;
            ImageBarr[i].dstAccessMask = (VkAccessFlags)barrier.AccessFlagsAfter;
            ImageBarr[i].srcQueueFamilyIndex = QueueFamilyInd(device.retrieve_as_forced<vDevice>(), barrier.previousQueue);
            ImageBarr[i].dstQueueFamilyIndex = QueueFamilyInd(device.retrieve_as_forced<vDevice>(), barrier.nextQueue);
            VkImageSubresourceRange range;
            range.aspectMask = (VkImageAspectFlags)barrier.subresourceRange.imageAspect;
            range.baseMipLevel = barrier.subresourceRange.IndexOrFirstMipLevel;
            range.baseArrayLayer = barrier.subresourceRange.FirstArraySlice;
            range.layerCount = barrier.subresourceRange.NumArraySlices;
            range.levelCount = barrier.subresourceRange.NumMipLevels;
            ImageBarr[i].subresourceRange = range;
            if (ImageBarr[i].srcQueueFamilyIndex == ImageBarr[i].dstQueueFamilyIndex)
                ImageBarr[i].srcQueueFamilyIndex = ImageBarr[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        }
        vkCmdPipelineBarrier((VkCommandBuffer)ID, (VkFlags)syncBefore, (VkFlags)syncAfter, 0, 0, nullptr, bufferBarriers.size(), BufferBarr, textureBarriers.size(), ImageBarr);
    }
    void GraphicsCommandList::BeginRendering(const RenderingBeginDesc& desc)
    {
        VkRenderingAttachmentInfo Attachmentinfos[5] = {};
        for (uint32_t i = 0; i < desc.numColorAttachments; i++)
        {
            Attachmentinfos[i].clearValue.color = *(VkClearColorValue*)&desc.pColorAttachments[i].clearColor;
            Attachmentinfos[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            Attachmentinfos[i].imageView = *(VkImageView*)desc.pColorAttachments[i].ImageView.ptr;
            Attachmentinfos[i].loadOp = vloadOp(desc.pColorAttachments[i].loadOp);
            Attachmentinfos[i].storeOp = vStoreOp(desc.pColorAttachments[i].storeOp);
            Attachmentinfos[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        }
        VkRect2D render_area;
        render_area.offset = { desc.renderingArea.offset.x,desc.renderingArea.offset.y };
        render_area.extent = { desc.renderingArea.size.x,desc.renderingArea.size.y };
        VkRenderingInfo info = {};
        info.pColorAttachments = Attachmentinfos;
        VkRenderingAttachmentInfo depthAttach{};
        if (desc.pDepthStencilAttachment)
        {
            depthAttach.clearValue.color = *(VkClearColorValue*)&desc.pDepthStencilAttachment->clearColor;
            depthAttach.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttach.imageView = *(VkImageView*)desc.pDepthStencilAttachment->ImageView.ptr;
            depthAttach.loadOp = vloadOp(desc.pDepthStencilAttachment->loadOp);
            depthAttach.storeOp = vStoreOp(desc.pDepthStencilAttachment->storeOp);
            depthAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            info.pDepthAttachment = &depthAttach;
        }
        info.colorAttachmentCount = desc.numColorAttachments;
        info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        info.renderArea = render_area;
        info.layerCount = 1;
        vkCmdBeginRenderingKHR((VkCommandBuffer)ID, &info);
    }
    void GraphicsCommandList::PushConstants(uint32_t bindingIndex,uint32_t numConstants,const void* constants, uint32_t offsetIn32BitSteps)
    {
        Weak<vRootSignature> rs = reinterpret_cast<vGraphicsCommandList*>(this)->currentRS;
        vkCmdPushConstants((VkCommandBuffer)ID,(VkPipelineLayout)rs->ID,rs->pcBindingToStage[bindingIndex], offsetIn32BitSteps * sizeof(uint32_t), numConstants * sizeof(uint32_t), constants);
    }
    void GraphicsCommandList::PushConstant(uint32_t bindingIndex, uint32_t constant, uint32_t offsetIn32BitSteps)
    {
        Weak<vRootSignature> rs = reinterpret_cast<vGraphicsCommandList*>(this)->currentRS;
        vkCmdPushConstants(static_cast<VkCommandBuffer>(ID), static_cast<VkPipelineLayout>(rs->ID), rs->pcBindingToStage[bindingIndex], offsetIn32BitSteps * sizeof(uint32_t), sizeof(uint32_t), &constant);
    }
    void GraphicsCommandList::EndRendering()
    {
        vkCmdEndRenderingKHR((VkCommandBuffer)ID);
    }
    RESULT GraphicsCommandList::End()
    {
        return vkEndCommandBuffer((VkCommandBuffer)ID);
    }
    void GraphicsCommandList::SetPipelineState(Weak<PipelineStateObject> pso)
    {
        vkCmdBindPipeline((VkCommandBuffer)ID, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)pso->ID);
    }
    void GraphicsCommandList::SetScissorRects(uint32_t numRects, Area2D* rects)
    {
        VkRect2D scr[4];
        for (uint32_t i = 0; i < numRects; i++)
        {
            scr[i].extent = { rects[i].size.x, rects[i].size.y};
            scr[i].offset = { rects[i].offset.x,rects[i].offset.y};
        }
        vkCmdSetScissor((VkCommandBuffer)ID, 0, numRects, scr);
    }
    void GraphicsCommandList::SetViewports(uint32_t numViewports, Viewport* viewports)
    {
        VkViewport vp[4];
        for (uint32_t i = 0; i < numViewports; i++)
        {
            vp[i].x = viewports[i].x;
            vp[i].height = -viewports[i].height + viewports[i].y;
            vp[i].y = viewports[i].height + viewports[i].y;
            vp[i].width = viewports[i].width;
            vp[i].minDepth = viewports[i].minDepth;
            vp[i].maxDepth = viewports[i].maxDepth;
        }
        vkCmdSetViewport((VkCommandBuffer)ID, 0, numViewports, vp);
    }
    void GraphicsCommandList::Draw(uint32_t numVertices, uint32_t numInstances, uint32_t firstVertex, uint32_t firstIndex)
    {
        vkCmdDraw((VkCommandBuffer)ID, numVertices, numInstances, firstVertex, firstIndex);
    }
    void GraphicsCommandList::BindVertexBuffers(uint32_t startSlot, uint32_t numBuffers, const Internal_ID* buffers)
    {
        VkDeviceSize l = 0;
        vkCmdBindVertexBuffers((VkCommandBuffer)ID, startSlot, numBuffers, (VkBuffer*)buffers, &l);
    }
    void GraphicsCommandList::SetRootSignature(Weak<RootSignature> rs)
    {
        ((vGraphicsCommandList*)this)->currentRS = rs.ForceTransform<vRootSignature>();
    }
    void GraphicsCommandList::BindDynamicDescriptor(Weak<DynamicDescriptor> set, std::uint32_t setIndex, std::uint32_t offset)
    {
        VkDescriptorSet sets;
        sets = (VkDescriptorSet)set->ID;
        Weak<vRootSignature> rs = ((vGraphicsCommandList*)this)->currentRS;
        vkCmdBindDescriptorSets((VkCommandBuffer)ID, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipelineLayout)rs->ID, setIndex, 1, &sets,1,&offset);
    }
    void GraphicsCommandList::BindDescriptorSet(Weak<DescriptorSet> set, std::uint32_t setIndex)
    {
        VkDescriptorSet sets;
        sets = (VkDescriptorSet)set->ID;
        Weak<vRootSignature> rs = ((vGraphicsCommandList*)this)->currentRS;
        vkCmdBindDescriptorSets((VkCommandBuffer)ID, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipelineLayout)rs->ID, setIndex, 1, &sets, 0, 0);
    }
    void GraphicsCommandList::SetDescriptorHeap(Weak<DescriptorHeap> heap)
    {
        //empty
    }
    void GraphicsCommandList::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
    {
        vkCmdDispatch((VkCommandBuffer)ID, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    }
    void GraphicsCommandList::BindIndexBuffer(Weak<Buffer> buffer, uint32_t offset)
    {
        vkCmdBindIndexBuffer((VkCommandBuffer)ID, (VkBuffer)buffer->ID, offset, VK_INDEX_TYPE_UINT32);
    }
    void GraphicsCommandList::BlitTexture(Weak<Texture> src, Weak<Texture> dst, Extent3D srcSize, Offset3D srcOffset, Extent3D dstSize, Offset3D dstOffset,SubResourceRange srcRange, SubResourceRange dstRange)
    {
        VkImageSubresourceLayers src_lyrs;
        src_lyrs.aspectMask = (VkImageAspectFlags)srcRange.imageAspect;
        src_lyrs.baseArrayLayer = srcRange.FirstArraySlice;
        src_lyrs.layerCount = srcRange.NumArraySlices;
        src_lyrs.mipLevel = srcRange.IndexOrFirstMipLevel;
        VkImageSubresourceLayers dst_lyrs;
        dst_lyrs.aspectMask = (VkImageAspectFlags)dstRange.imageAspect;
        dst_lyrs.baseArrayLayer = dstRange.FirstArraySlice;
        dst_lyrs.layerCount = dstRange.NumArraySlices;
        dst_lyrs.mipLevel = dstRange.IndexOrFirstMipLevel;
        VkImageBlit blt;
        blt.dstOffsets[0] = { dstOffset.width, dstOffset.height, dstOffset.depth };
        blt.dstOffsets[1] = { (int)(dstOffset.width + dstSize.width), (int)(dstOffset.height + dstSize.height),(int)(dstOffset.depth + dstSize.depth) };
        blt.srcOffsets[0] = { srcOffset.width, srcOffset.height, srcOffset.depth};
        blt.srcOffsets[1] =
        { (int)(srcOffset.width + srcSize.width), (int)(srcOffset.height + srcSize.height),(int)(srcOffset.depth + srcSize.depth)};
        blt.srcSubresource = src_lyrs;
        blt.dstSubresource = dst_lyrs;
        vkCmdBlitImage((VkCommandBuffer)ID, (VkImage)src->ID, VK_IMAGE_LAYOUT_GENERAL, (VkImage)dst->ID, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blt, VK_FILTER_LINEAR);
    }
    void GraphicsCommandList::MarkBuffer(Weak<Buffer> buffer, uint32_t offset, uint32_t val)
    {
        vkCmdFillBuffer((VkCommandBuffer)ID, (VkBuffer)buffer->ID, offset, sizeof(uint32_t), val);
    }
    void GraphicsCommandList::MarkBuffer(Weak<DebugBuffer> buffer, uint32_t val)
    {
        VkAfterCrash_CmdWriteMarker((VkCommandBuffer)ID, (VkAfterCrash_Buffer)buffer->ID, 0, val);
    }
    void GraphicsCommandList::DrawIndexed(uint32_t IndexCount, uint32_t InstanceCount, uint32_t startIndexLocation, uint32_t startVertexLocation, uint32_t startInstanceLocation)
    {
        vkCmdDrawIndexed((VkCommandBuffer)ID, IndexCount, InstanceCount, startIndexLocation, startVertexLocation, startInstanceLocation);
    }
    void GraphicsCommandList::CopyBufferRegion(uint32_t srcOffset, uint32_t dstOffset, uint32_t size, Weak<Buffer> srcBuffer, Weak<Buffer> dstBuffer)
    {
        VkBufferCopy copy{};
        copy.size = size;
        copy.srcOffset = srcOffset;
        copy.dstOffset = dstOffset;
        vkCmdCopyBuffer((VkCommandBuffer)ID, (VkBuffer)srcBuffer->ID, (VkBuffer)dstBuffer->ID, 1, &copy);
    }
    void GraphicsCommandList::CopyBufferToImage(uint32_t srcOffset, SubResourceRange dstRange, Offset3D imgOffset, Extent3D imgSize, Weak<Buffer> buffer, Weak<Texture> texture)
    {
        VkBufferImageCopy copy{};
        copy.bufferImageHeight = 0;
        copy.bufferOffset = srcOffset;
        copy.bufferRowLength = 0;
        copy.imageExtent = { imgSize.width, imgSize.height, imgSize.depth };
        copy.imageOffset = { imgOffset.width, imgOffset.height, imgOffset.depth };
        copy.imageSubresource.aspectMask = (VkImageAspectFlags)dstRange.imageAspect;
        copy.imageSubresource.baseArrayLayer = dstRange.FirstArraySlice;
        copy.imageSubresource.layerCount = dstRange.NumArraySlices;
        copy.imageSubresource.mipLevel = dstRange.IndexOrFirstMipLevel;
        vkCmdCopyBufferToImage((VkCommandBuffer)ID, (VkBuffer)buffer->ID, (VkImage)texture->ID, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
    }

    void GraphicsCommandList::CopyTextureRegion(SubResourceRange srcRange, SubResourceRange dstRange, Offset3D srcOffset, Offset3D dstOffset, Extent3D extent, Weak<Texture> src, Weak<Texture> dst)
    {
        VkImageCopy copy{};
        copy.dstOffset = { dstOffset.width, dstOffset.height,dstOffset.depth };
        copy.srcOffset = { srcOffset.width, srcOffset.height, dstOffset.depth };
        copy.extent = { extent.width,extent.height,extent.depth };
        copy.srcSubresource.aspectMask = (VkImageAspectFlags)srcRange.imageAspect;
        copy.srcSubresource.baseArrayLayer = srcRange.FirstArraySlice;
        copy.srcSubresource.layerCount = srcRange.NumArraySlices;
        copy.srcSubresource.mipLevel = srcRange.IndexOrFirstMipLevel;
        copy.dstSubresource.aspectMask = (VkImageAspectFlags)dstRange.imageAspect;
        copy.dstSubresource.baseArrayLayer = dstRange.FirstArraySlice;
        copy.dstSubresource.layerCount = dstRange.NumArraySlices;
        copy.dstSubresource.mipLevel = dstRange.IndexOrFirstMipLevel;
        vkCmdCopyImage((VkCommandBuffer)ID, (VkImage)src->ID, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, (VkImage)dst->ID, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
    }

    void GraphicsCommandList::ReleaseBarrier(PipelineStage syncBefore, PipelineStage syncAfter, std::span<BufferMemoryBarrier> bufferBarriers, std::span<TextureMemoryBarrier> textureBarriers)
    {
        VkBufferMemoryBarrier BufferBarr[10]{};
        VkImageMemoryBarrier ImageBarr[100]{};
        int bn = 0, tn = 0;
        for (uint32_t i = 0; i < bufferBarriers.size(); i++)
        {
            auto& barrier = bufferBarriers[i];
            if (QueueFamilyInd(device.retrieve_as_forced<vDevice>(), barrier.previousQueue) == QueueFamilyInd(device.retrieve_as_forced<vDevice>(), barrier.nextQueue))
                continue;
            BufferBarr[bn].buffer = (VkBuffer)barrier.buffer->ID;
            BufferBarr[bn].srcAccessMask = (VkAccessFlags)barrier.AccessFlagsBefore;
            BufferBarr[bn].dstAccessMask = (VkAccessFlags)barrier.AccessFlagsAfter;
            BufferBarr[bn].srcQueueFamilyIndex = QueueFamilyInd(device.retrieve_as_forced<vDevice>(), barrier.previousQueue);
            BufferBarr[bn].dstQueueFamilyIndex = QueueFamilyInd(device.retrieve_as_forced<vDevice>(), barrier.nextQueue);
            BufferBarr[bn].offset = barrier.offset;
            BufferBarr[bn].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            BufferBarr[bn].size = barrier.size;
            bn++;
        }
        for (uint32_t i = 0; i < textureBarriers.size(); i++)
        {
            auto& barrier = textureBarriers[i];
            if(QueueFamilyInd(device.retrieve_as_forced<vDevice>(), barrier.previousQueue) == QueueFamilyInd(device.retrieve_as_forced<vDevice>(), barrier.nextQueue))
                continue;
            ImageBarr[tn].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            ImageBarr[tn].image = (VkImage)barrier.texture->ID;
            ImageBarr[tn].newLayout = (VkImageLayout)barrier.newLayout;
            ImageBarr[tn].oldLayout = (VkImageLayout)barrier.oldLayout;
            ImageBarr[tn].srcAccessMask = (VkAccessFlags)barrier.AccessFlagsBefore;
            ImageBarr[tn].dstAccessMask = (VkAccessFlags)barrier.AccessFlagsAfter;
            ImageBarr[tn].srcQueueFamilyIndex = QueueFamilyInd(device.retrieve_as_forced<vDevice>(), barrier.previousQueue);
            ImageBarr[tn].dstQueueFamilyIndex = QueueFamilyInd(device.retrieve_as_forced<vDevice>(), barrier.nextQueue);
            VkImageSubresourceRange range;
            range.aspectMask = (VkImageAspectFlags)barrier.subresourceRange.imageAspect;
            range.baseMipLevel = barrier.subresourceRange.IndexOrFirstMipLevel;
            range.baseArrayLayer = barrier.subresourceRange.FirstArraySlice;
            range.layerCount = barrier.subresourceRange.NumArraySlices;
            range.levelCount = barrier.subresourceRange.NumMipLevels;
            ImageBarr[tn].subresourceRange = range;
            tn++;
        }
        if(bn+tn)
            vkCmdPipelineBarrier((VkCommandBuffer)ID, (VkFlags)syncBefore, (VkFlags)syncAfter, 0, 0, nullptr, bn, BufferBarr, tn, ImageBarr);
    }

    void GraphicsCommandList::SetComputePipeline(Weak<ComputePipeline> cp)
    {
        vkCmdBindPipeline((VkCommandBuffer)ID, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipeline)cp->ID);
    }
    void GraphicsCommandList::BindComputeDescriptorSet(Weak<DescriptorSet> set, std::uint32_t setIndex)
    {
        VkDescriptorSet sets;
        sets = (VkDescriptorSet)set->ID;
        Weak<vRootSignature> rs = ((vGraphicsCommandList*)this)->currentRS;
        vkCmdBindDescriptorSets((VkCommandBuffer)ID, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipelineLayout)rs->ID, setIndex, 1, &sets, 0, 0);
    }
    void GraphicsCommandList::BindComputeDynamicDescriptor(Weak<DynamicDescriptor> set, std::uint32_t setIndex, std::uint32_t offset)
    {
        VkDescriptorSet sets;
        sets = (VkDescriptorSet)set->ID;
        Weak<vRootSignature> rs = ((vGraphicsCommandList*)this)->currentRS;
        vkCmdBindDescriptorSets((VkCommandBuffer)ID, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipelineLayout)rs->ID, setIndex, 1, &sets, 1, &offset);
    }
}
