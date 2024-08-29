#include "pch.h"
#include "../SwapChain.h"
#include "volk.h"
#include "VulkanSpecific.h"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace RHI
{
	SwapChainDesc::SwapChainDesc(Default_t)
	{
		RefreshRate = { 60, 1 };
		BufferCount = 2;
		Flags = 0;
		Width = 0;
		Height = 0;
		OutputSurface = Surface();
		SampleCount = 1;
		SampleQuality = 0;
		SwapChainFormat = RHI::Format::R8G8B8A8_UNORM;
		Windowed = true;
	}
	SwapChainDesc::SwapChainDesc(Zero_t)
	{
		memset(this, 0,sizeof(SwapChainDesc));
	}
	SwapChainDesc::SwapChainDesc()
	{
	}
	/*
	Wait for the previous acquired image to be ready and for the passed Fence to be completed
	Present and acquire the next image to be presented
	*/
	RESULT SwapChain::Present(Weak<Fence> wait, uint64_t val)
	{
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
		vkQueueSubmit(vchain->PresentQueue_ID, 1, &submitInfo, VK_NULL_HANDLE);

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = 0;
		presentInfo.pWaitSemaphores = nullptr;
		VkSwapchainKHR swapChains[] = { (VkSwapchainKHR)ID };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &vchain->imgIndex;
		vkWaitForFences(dev, 1, &vchain->imageAcquired, true, UINT64_MAX);

		vkQueuePresentKHR(((vSwapChain*)this)->PresentQueue_ID, &presentInfo);
		vkResetFences(dev, 1, &vchain->imageAcquired);
		vkAcquireNextImageKHR(dev, (VkSwapchainKHR)ID, UINT64_MAX, VK_NULL_HANDLE, vchain->imageAcquired, &vchain->imgIndex);
		return RESULT();
	}
	uint32_t SwapChain::GetImageIndex()
	{
		return ((vSwapChain*)this)->imgIndex;
	}
}
