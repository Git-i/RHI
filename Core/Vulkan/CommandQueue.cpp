#include "pch.h"
#include "../CommandQueue.h"
#include "VulkanSpecific.h"
#include "volk.h"
namespace RHI
{
	RESULT CommandQueue::SignalFence(Weak<Fence> fence, std::uint64_t val)
	{
		std::uint64_t fenceVal = val;
		VkTimelineSemaphoreSubmitInfo timelineInfo = {};
		timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timelineInfo.pNext = NULL;
		timelineInfo.signalSemaphoreValueCount = 1;
		timelineInfo.pSignalSemaphoreValues = &fenceVal;
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = &timelineInfo;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = (VkSemaphore*)&fence->ID;
		return vkQueueSubmit((VkQueue)ID, 1, &submitInfo, VK_NULL_HANDLE);
	}
	RESULT CommandQueue::ExecuteCommandLists(const Internal_ID* lists, std::uint32_t count)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = 0;
		submitInfo.commandBufferCount = count;
		submitInfo.pCommandBuffers = (VkCommandBuffer*)lists;
		return vkQueueSubmit((VkQueue)ID, 1, &submitInfo, VK_NULL_HANDLE);
	}
	RESULT CommandQueue::WaitForFence(Weak<Fence> fence, std::uint64_t val)
	{
		std::uint64_t value = val;
		VkTimelineSemaphoreSubmitInfo timelineInfo{};
		timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timelineInfo.pNext = NULL;
		timelineInfo.waitSemaphoreValueCount = 1;
		timelineInfo.pWaitSemaphoreValues = &value;
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = &timelineInfo;
		submitInfo.waitSemaphoreCount = 1;
		VkPipelineStageFlags fg = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		submitInfo.pWaitDstStageMask = &fg;
		submitInfo.pWaitSemaphores = (VkSemaphore*)&fence->ID;
		return vkQueueSubmit((VkQueue)ID, 1, &submitInfo, VK_NULL_HANDLE);
	}
	CommandQueue* CommandQueue::FromNativeHandle(Internal_ID id)
	{
		RHI::vCommandQueue* vQueue = new RHI::vCommandQueue;
		vQueue->ID = id;
		return vQueue;
	}
}
