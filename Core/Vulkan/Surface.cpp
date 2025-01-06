#include "Core.h"
#include "pch.h"
#include "volk.h"
#include "../Surface.h"
#include "VulkanSpecific.h"
namespace RHI
{
	#ifdef WIN32
	creation_result<Surface> Surface::InitWin32(HWND hwnd, Ptr<Instance> instance)
	{
		Ptr<Surface> srf(new vSurface);
		VkWin32SurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = hwnd;
		createInfo.hinstance = GetModuleHandle(nullptr);
		auto res = vkCreateWin32SurfaceKHR((VkInstance)instance->ID, &createInfo, nullptr, reinterpret_cast<VkSurfaceKHR*>(&srf->ID));
		if (res != VK_SUCCESS) return ezr::err(marshall_error(res));
		srf.retrieve_as_forced<vSurface>()->instance = instance;
		return srf;
	}
	#endif
	#ifdef __linux__
	//void RHI::Surface::InitLinux()
	//{
	//}
	#endif
	#ifdef USE_GLFW

	creation_result<Surface> Surface::InitGLFW(GLFWwindow* window, Ptr<Instance> instance)
	{
		Ptr<Surface> srf(new vSurface);
		VkResult res = glfwCreateWindowSurface(static_cast<VkInstance>(instance->ID), window, nullptr,
			reinterpret_cast<VkSurfaceKHR*>(&srf->ID));
		if (res != VK_SUCCESS) return ezr::err(marshall_error(res));
		srf.retrieve_as_forced<vSurface>()->instance = instance;
		return srf;
	}
	#endif
}