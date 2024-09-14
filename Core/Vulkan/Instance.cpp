#include "FormatsAndTypes.h"
#include "PhysicalDevice.h"
#include "Ptr.h"
#include "SwapChain.h"
#include "../Instance.h"
#include "Core.h"
#include "volk.h"
#include "VulkanSpecific.h"
#include <vector>
#include <iostream>
#include <array>
#include <vulkan/vulkan_core.h>
#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {
	if(pCallbackData->messageIdNumber == 0 || pCallbackData->messageIdNumber == -937765618) return VK_FALSE;
	std::string_view view(pCallbackData->pMessage);
	char id_string[1024];
	strcpy(id_string, "MessageID = 0x");
	sprintf(id_string + strlen(id_string), "%x", pCallbackData->messageIdNumber);
	size_t ind = view.find(id_string);
	ind += strlen(id_string);
	std::cerr << std::endl << "validation layer" << pCallbackData->pMessage + ind << std::endl;
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT && pCallbackData->messageIdNumber != -937765618 && pCallbackData->messageIdNumber!=-1831440447)// && pCallbackData->messageIdNumber != 0x4dae5635)
	{
		//DEBUG_BREAK;
	}

	return VK_FALSE;
}
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))
#endif
extern "C"
{
	RESULT RHI_API RHICreateInstance(RHI::Instance** instance)
	{
		if(RHI::vInstance::current)
		{
			return -1;
		}
		RHI::vInstance* vinstance = new RHI::vInstance;
		*instance = vinstance;
		VkResult res = volkInitialize();
		if(res != VK_SUCCESS) return res;
		VkValidationFeatureEnableEXT enabled[] =
		{
			//VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
		};
		VkValidationFeaturesEXT features{};
		features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		features.pEnabledValidationFeatures = enabled;
		features.enabledValidationFeatureCount = sizeof(enabled) / sizeof(VkValidationFeatureEnableEXT);
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "NULL";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "NULL";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;
		VkInstanceCreateInfo info = {};
		const char* layerNames[] = 
		{
			"VK_LAYER_KHRONOS_validation",
		};
		std::vector<const char*> extensionName = { "VK_KHR_surface",VK_EXT_DEBUG_UTILS_EXTENSION_NAME,VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME};
		#ifdef VK_USE_PLATFORM_WIN32_KHR
			extensionName.push_back("VK_KHR_win32_surface");
		#elif defined(USE_GLFW)
			uint32_t count;
			const char ** ext = glfwGetRequiredInstanceExtensions(&count);
			for(uint32_t i = 0; i < count; i++) extensionName.push_back(ext[i]);
		#endif
		info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		info.pNext = &features;
		info.enabledLayerCount = ARRAYSIZE(layerNames);
		info.ppEnabledLayerNames = layerNames;
		info.enabledExtensionCount = extensionName.size();
		info.ppEnabledExtensionNames = extensionName.data();
		info.pApplicationInfo = &appInfo;
		res = vkCreateInstance(&info, nullptr, (VkInstance*)&vinstance->ID);
		volkLoadInstance((VkInstance)vinstance->ID);
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional
		vkCreateDebugUtilsMessengerEXT((VkInstance)vinstance->ID, &createInfo, nullptr, &vinstance->messanger);
		RHI::vInstance::current = vinstance;
		return res;
	}
}
namespace RHI
{
	RHI::Instance* Instance::FromNativeHandle(Internal_ID id)
	{
		RHI::vInstance* inst = new RHI::vInstance;
		inst->ID = id;
		volkInitialize();
		volkLoadInstance((VkInstance)inst->ID);
		return inst;
	}
	RESULT Instance::GetPhysicalDevice(uint32_t id, PhysicalDevice** device)
	{
		vPhysicalDevice* vdevice = new vPhysicalDevice;
		std::vector<VkPhysicalDevice> devices;
		std::uint32_t count;
		vkEnumeratePhysicalDevices((VkInstance)ID, &count, nullptr);
		devices.resize(count);
		VkResult res = vkEnumeratePhysicalDevices((VkInstance)ID, &count, &devices[0]);
		vdevice->ID = devices[id];
		*device = vdevice;
		return res;
	}
	void Instance::SetLoggerCallback(const std::function<void(LogLevel, std::string_view)>& fn)
	{	
		((vInstance*)this)->logCallback = fn;
	}
	RESULT Instance::GetAllPhysicalDevices(PhysicalDevice** devices)
	{
		std::uint32_t count;
		vkEnumeratePhysicalDevices((VkInstance)ID, &count, nullptr);
		std::vector<VkPhysicalDevice> vk_devices(count);
		VkResult res = vkEnumeratePhysicalDevices((VkInstance)ID, &count, vk_devices.data());
		uint32_t i = 0;
		for(auto dev : vk_devices)
		{
			devices[i] = new vPhysicalDevice;
			devices[i]->ID = dev;
			i++;
		}
		return res;
	}
	API Instance::GetInstanceAPI()
	{
		return API::Vulkan;
	}
	uint32_t Instance::GetNumPhysicalDevices()
	{
		std::uint32_t count;
		vkEnumeratePhysicalDevices((VkInstance)ID, &count, nullptr);
		return count;
	}
	std::pair<uint32_t, uint32_t> Instance::GetSwapChainMinMaxImageCount(PhysicalDevice* pDev, Surface* surface)
	{
		VkSurfaceCapabilitiesKHR caps;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR((VkPhysicalDevice)pDev->ID,(VkSurfaceKHR)surface->ID,&caps);
		return {caps.minImageCount, caps.maxImageCount};
	}
	creation_result<SwapChain> Instance::CreateSwapChain(const SwapChainDesc& desc, PhysicalDevice* pDevice, Ptr<Device> Device, Weak<CommandQueue> pCommandQueue)
	{
		Ptr<vSwapChain> vswapChain(new vSwapChain);
		VkSwapchainCreateInfoKHR createInfo{};
		std::uint32_t count;
		vkGetPhysicalDeviceSurfaceFormatsKHR((VkPhysicalDevice)pDevice->ID, (VkSurfaceKHR)desc.OutputSurface.ID, &count, nullptr);
		std::vector<VkSurfaceFormatKHR> format(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR((VkPhysicalDevice)pDevice->ID, (VkSurfaceKHR)desc.OutputSurface.ID, &count, format.data());

		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = (VkSurfaceKHR)desc.OutputSurface.ID;

		createInfo.minImageCount = desc.BufferCount;
		createInfo.imageFormat = FormatConv(desc.SwapChainFormat);
		createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		createInfo.imageExtent = { desc.Width, desc.Height };
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		auto [indices,_] = findQueueFamilyIndices(pDevice, desc.OutputSurface);
		uint32_t queueFamilyIndices[] = { indices.graphicsIndex, indices.presentIndex };

		if (indices.graphicsIndex != indices.presentIndex) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;
		VkResult res = vkCreateSwapchainKHR((VkDevice)Device->ID, &createInfo, nullptr, (VkSwapchainKHR*)&vswapChain->ID);
		if(res < 0) return ezr::err(marshall_error(res));
		VkFenceCreateInfo finfo{};
		finfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		finfo.flags = 0;
		res = vkCreateFence((VkDevice)Device->ID,&finfo,0,&vswapChain->imageAcquired);
		if(res < 0) return ezr::err(marshall_error(res));
		vkAcquireNextImageKHR((VkDevice)Device->ID, (VkSwapchainKHR)vswapChain->ID, UINT64_MAX, VK_NULL_HANDLE, vswapChain->imageAcquired, &vswapChain->imgIndex);
		vswapChain->device = Device;
		Device->Release();
		vkGetDeviceQueue((VkDevice)Device->ID, indices.presentIndex, 0, &vswapChain->PresentQueue_ID);
		VkDebugUtilsObjectNameInfoEXT name_info{};
		name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		name_info.pObjectName = "Internal Swapchain Fence";
		name_info.objectHandle = (uint64_t)vswapChain->imageAcquired;
		name_info.objectType = VK_OBJECT_TYPE_FENCE;
		vkSetDebugUtilsObjectNameEXT((VkDevice)Device->ID, &name_info);
		return creation_result<SwapChain>::ok(vswapChain);
	}
}