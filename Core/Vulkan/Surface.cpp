#include "Core.h"
#include "pch.h"
#include "volk.h"
#include "../Surface.h"
namespace RHI
{
	#ifdef WIN32
	void RHI::Surface::InitWin32(HWND hwnd, Internal_ID instance)
	{
		VkWin32SurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = hwnd;
		createInfo.hinstance = GetModuleHandle(nullptr);
		vkCreateWin32SurfaceKHR((VkInstance)instance, &createInfo, nullptr, (VkSurfaceKHR*)&ID);
	}
	#endif
	#ifdef __linux__
	//void RHI::Surface::InitLinux()
	//{
	//}
	#endif
	#ifdef USE_GLFW
	void RHI::Surface::InitGLFW(GLFWwindow* window, Internal_ID instance)
	{
		VkResult res = glfwCreateWindowSurface((VkInstance)instance, window, nullptr, (VkSurfaceKHR*)&ID);
	}
	#endif
}