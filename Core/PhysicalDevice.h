#pragma once
#include "Object.h"
namespace RHI
{
	struct LUID
	{
		uint8_t data[8] = {};
	};
	enum class DeviceType
	{
		Integrated, Dedicated, CPU
	};
	struct PhysicalDeviceDesc
	{
		char  Description[128];
		std::size_t DedicatedVideoMemory;
		std::size_t SharedSystemMemory;
		DeviceType type;
		LUID   AdapterLuid;
	};
	class RHI_API PhysicalDevice
	{
	public:
		static RHI::PhysicalDevice* FromNativeHandle(Internal_ID id);
		PhysicalDeviceDesc GetDesc();
		Internal_ID ID;
	};

}
