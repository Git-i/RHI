#include "pch.h"
#include "../PhysicalDevice.h"
#include "VulkanSpecific.h"
#include <string>
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
}
