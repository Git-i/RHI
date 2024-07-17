#include "pch.h"
#include "../SwapChain.h"
#include "volk.h"
#include "VulkanSpecific.h"
#include <vector>
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
	RESULT SwapChain::Present(std::uint32_t imgIndex, std::uint32_t cycle)
	{
		auto vchain = ((vSwapChain*)this);
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &vchain->present_semaphore[cycle];
		VkSwapchainKHR swapChains[] = { (VkSwapchainKHR)ID };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imgIndex;
		vkQueuePresentKHR(((vSwapChain*)this)->PresentQueue_ID, &presentInfo);
		return RESULT();
	}
	RESULT SwapChain::AcquireImage(std::uint32_t* imgIndex, uint32_t cycle)
	{
		auto vchain = ((vSwapChain*)this);
		VkResult res = vkAcquireNextImageKHR((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkSwapchainKHR)ID, UINT64_MAX, vchain->present_semaphore[cycle], VK_NULL_HANDLE, imgIndex);
		if(res != VK_SUCCESS) DEBUG_BREAK;
		return 0;
	}
}
