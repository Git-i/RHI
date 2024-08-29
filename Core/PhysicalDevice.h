#pragma once
#include "Object.h"
#include <array>
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
	/**
	 * Structure Specifying Queue types and amount supported by a device
	 * 
	 * Because queues multiple queue operations may be available on the same queue the 
	 * counts of available queues for each type can be shared, all the shared counts are stored in the `counts` array
	 * where the right count for a specified operation type can be retireived by indexing the array with the appropriate
	 * count index variable. This also implies that if `gfxCountIndex` == `cmpCountIndex` they are from a shared "pool" of queues,
	 * same goes for any combination of these indices.
	 */
	struct QueueInfo
	{
		std::array<uint32_t, 3> counts;
		bool graphicsSupported;
		bool computeSupported;
		bool copySupported;
		uint32_t gfxCountIndex;
		uint32_t cmpCountIndex;
		uint32_t copyCountIndex;
	};
	class RHI_API PhysicalDevice
	{
	public:
		static RHI::PhysicalDevice* FromNativeHandle(Internal_ID id);
		QueueInfo GetQueueInfo();
		PhysicalDeviceDesc GetDesc();
		Internal_ID ID;
	};

}
