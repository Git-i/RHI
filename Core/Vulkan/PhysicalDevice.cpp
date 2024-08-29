#include "FormatsAndTypes.h"
#include "../PhysicalDevice.h"
#include "VulkanSpecific.h"
#include <cstdint>
#include <unordered_map>
#include <vulkan/vulkan_core.h>
namespace RHI
{
	RHI::PhysicalDevice* PhysicalDevice::FromNativeHandle(Internal_ID id)
	{
		vPhysicalDevice* dev = new vPhysicalDevice;
		dev->ID = id;
		return dev;
	}
	DeviceType ToRHIType(VkPhysicalDeviceType t)
	{
		using enum DeviceType;
		switch(t)
		{
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return Integrated;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return Dedicated;
			case VK_PHYSICAL_DEVICE_TYPE_CPU: return CPU;
			default: return Unknown;
		}
	}
    PhysicalDeviceDesc PhysicalDevice::GetDesc()
    {
		PhysicalDeviceDesc returnVal;
		VkPhysicalDeviceIDProperties id_props{};
		id_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
		VkPhysicalDeviceProperties2 result;
		result.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		result.pNext = &id_props;
		vkGetPhysicalDeviceProperties2((VkPhysicalDevice)ID, &result);
		memcpy(returnVal.Description, result.properties.deviceName, 128);
		memcpy(returnVal.AdapterLuid.data, id_props.deviceLUID, sizeof(uint8_t) * 8);
		returnVal.type =ToRHIType( result.properties.deviceType);
		return returnVal;
    }
	QueueInfo PhysicalDevice::GetQueueInfo()
	{
		auto[family, count] = findQueueFamilyIndices(this);
		std::unordered_map<uint32_t, uint32_t> family_to_count_index;
		QueueInfo info;
		uint32_t next_count_index = 0;
		if(family.flags & HasGraphics)
		{
			info.graphicsSupported = true;
			info.gfxCountIndex = next_count_index;
			info.counts[next_count_index] = count[family.graphicsIndex];
			family_to_count_index[family.graphicsIndex] = next_count_index++;
			
		}
		if(family.flags & HasCompute)
		{
			info.computeSupported = true;
			uint32_t ci;
			if(family_to_count_index.contains(family.computeIndex)) ci = family_to_count_index[family.computeIndex];
			else ci = next_count_index++;
			info.counts[ci] = count[family.computeIndex];
			info.cmpCountIndex = ci;
			family_to_count_index[family.computeIndex] = ci;
		}
		if(family.flags & HasCopy)
		{
			info.copySupported = true;
			uint32_t ci;
			if(family_to_count_index.contains(family.copyIndex)) ci = family_to_count_index[family.copyIndex];
			else ci = next_count_index++;
			info.counts[ci] = count[family.copyIndex];
			info.copyCountIndex = ci;
		}
		return info;
	}
}
