#pragma once
#include "Core.h"
#include "FormatsAndTypes.h"
#include "PhysicalDevice.h"
#include "SwapChain.h"
#include "Device.h"
#include <functional>
#include <string_view>
namespace RHI
{
	enum class LogLevel
	{
		Error, Warn, Info
	};
	class RHI_API Instance : public Object
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(Instance);
	public:
		void SetLoggerCallback(const std::function<void(LogLevel, std::string_view)>&);
		API GetInstanceAPI();
		uint32_t GetNumPhysicalDevices();
		std::pair<uint32_t, uint32_t> GetSwapChainMinMaxImageCount(PhysicalDevice* pDev, Surface* surface);
		RESULT GetAllPhysicalDevices(PhysicalDevice** devices);
		RESULT GetPhysicalDevice(uint32_t id, PhysicalDevice** device);
		static creation_result<RHI::Instance> FromNativeHandle(Internal_ID id);
		creation_result<SwapChain> CreateSwapChain(const SwapChainDesc& desc, PhysicalDevice* device, Ptr<Device> pDevice, Weak<CommandQueue> pCommandQueue);
		static creation_result<Instance> Create();
	};

}
