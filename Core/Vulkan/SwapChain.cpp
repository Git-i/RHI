#include "Core.h"
#include "pch.h"
#include "../SwapChain.h"
#include "volk.h"
#include "VulkanSpecific.h"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace RHI
{
	SwapChainError marshall_swap_error(VkResult r)
	{
		switch (r) 
		{
			using enum SwapChainError;
			case VK_ERROR_OUT_OF_DATE_KHR: return OutOfDate;
			case VK_ERROR_OUT_OF_DEVICE_MEMORY: [[fallthrough]];
			case VK_ERROR_OUT_OF_HOST_MEMORY: return OutOfMemory;
			case VK_ERROR_DEVICE_LOST: return DeviceLost;
			case VK_SUBOPTIMAL_KHR: return SubOptimal;
			default: return Unknown;
		}
	}
	/*
	Wait for the previous acquired image to be ready and for the passed Fence to be completed
	Present and acquire the next image to be presented
	*/
	SwapChainError SwapChain::Present(Weak<Fence> wait, uint64_t val)
	{
		VkResult res;
		auto vchain = ((vSwapChain*)this);
		auto dev = (VkDevice)(device.retrieve_as_forced<vDevice>())->ID;
		VkTimelineSemaphoreSubmitInfo timelineInfo{};
		timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timelineInfo.pNext = NULL;
		timelineInfo.waitSemaphoreValueCount = 1;
		timelineInfo.pWaitSemaphoreValues = &val;
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = &timelineInfo;
		submitInfo.waitSemaphoreCount = 1;
		VkPipelineStageFlags fg = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		submitInfo.pWaitDstStageMask = &fg;
		submitInfo.pWaitSemaphores = (VkSemaphore*)&wait->ID;
		res = vkQueueSubmit(vchain->PresentQueue_ID, 1, &submitInfo, VK_NULL_HANDLE);
		if(res != VK_SUCCESS) return marshall_swap_error(res);
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = 0;
		presentInfo.pWaitSemaphores = nullptr;
		VkSwapchainKHR swapChains[] = { (VkSwapchainKHR)ID };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &vchain->imgIndex;
		
		res = vkWaitForFences(dev, 1, &vchain->imageAcquired, true, UINT64_MAX);
		if(res != VK_SUCCESS) return marshall_swap_error(res);
		
		res = vkQueuePresentKHR(((vSwapChain*)this)->PresentQueue_ID, &presentInfo);
		if(res != VK_SUCCESS) return marshall_swap_error(res);
		
		res = vkResetFences(dev, 1, &vchain->imageAcquired);
		if(res != VK_SUCCESS) return marshall_swap_error(res);

		res = vkAcquireNextImageKHR(dev, (VkSwapchainKHR)ID, UINT64_MAX, VK_NULL_HANDLE, vchain->imageAcquired, &vchain->imgIndex);
		if(res != VK_SUCCESS) return marshall_swap_error(res);
		return SwapChainError::None;
	}
	uint32_t SwapChain::GetImageIndex()
	{
		return ((vSwapChain*)this)->imgIndex;
	}
}
