#include "pch.h"
#include "VulkanSpecific.h"
#include "../Buffer.h"
#include "volk.h"
namespace RHI
{
	RESULT Buffer::Map(void** data)
	{
		auto vbuffer = reinterpret_cast<vBuffer*>(this);
		if (vbuffer->is_auto())
		{
			return vmaMapMemory((device.retrieve_as_forced<vDevice>())->allocator, vbuffer->vma_allocation(), data);
		}
		return vkMapMemory(static_cast<VkDevice>(device->ID), static_cast<VkDeviceMemory>(vbuffer->placed_data().heap->ID), vbuffer->placed_data().offset, vbuffer->placed_data().size, 0, data);
	}
	RESULT Buffer::UnMap()
	{
		auto vbuffer = reinterpret_cast<vBuffer*>(this);
		if (vbuffer->is_auto())
		{
			vmaUnmapMemory((device.retrieve_as_forced<vDevice>())->allocator, vbuffer->vma_allocation());
			return 0;
		}
		vkUnmapMemory(static_cast<VkDevice>(device->ID), static_cast<VkDeviceMemory>(vbuffer->placed_data().heap->ID));
		return 0;
	}
}

