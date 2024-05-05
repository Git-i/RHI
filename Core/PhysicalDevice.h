#pragma once
#include "Object.h"
namespace RHI
{
	struct LUID
	{
		uint8_t data[8] = {};
	};
	struct PhysicalDeviceDesc
	{
		wchar_t  Description[128];
		std::size_t DedicatedVideoMemory;
		std::size_t SharedSystemMemory;
		LUID   AdapterLuid;
	};
	class RHI_API PhysicalDevice
	{
	public:
		RESULT GetDesc(PhysicalDeviceDesc* desc);
		Internal_ID ID;;
	};

}
