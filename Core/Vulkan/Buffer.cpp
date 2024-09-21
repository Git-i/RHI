#include "pch.h"
#include "VulkanSpecific.h"
#include "../Buffer.h"
#include "volk.h"
namespace RHI
{
	RESULT Buffer::Map(void** data)
	{
		auto vbuffer = reinterpret_cast<vBuffer*>(this);
		if (vbuffer->vma_ID)
		{
			return vmaMapMemory((device.retrieve_as_forced<vDevice>())->allocator, vbuffer->vma_ID, data);
		}
		return vkMapMemory(static_cast<VkDevice>(device->ID), static_cast<VkDeviceMemory>(vbuffer->heap->ID), vbuffer->offset, vbuffer->size, 0, data);
	}
	RESULT Buffer::UnMap()
	{
		auto vbuffer = reinterpret_cast<vBuffer*>(this);
		if (vbuffer->vma_ID)
		{
			vmaUnmapMemory((device.retrieve_as_forced<vDevice>())->allocator, vbuffer->vma_ID);
			return 0;
		}
		vkUnmapMemory(static_cast<VkDevice>(device->ID), static_cast<VkDeviceMemory>(vbuffer->heap->ID));
		return 0;
	}
}

