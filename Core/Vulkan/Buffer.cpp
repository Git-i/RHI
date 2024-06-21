#include "pch.h"
#include "VulkanSpecific.h"
#include "../Buffer.h"
#include "volk.h"
namespace RHI
{
	RESULT Buffer::Map(void** data)
	{
		auto vbuffer = (vBuffer*)this;
		if (vbuffer->vma_ID)
		{
			return vmaMapMemory((device.retrieve_as<vDevice>())->allocator, vbuffer->vma_ID, data);
		}
		return vkMapMemory((VkDevice)(device.retrieve_as<vDevice>())->ID, (VkDeviceMemory)vbuffer->heap->ID, vbuffer->offset, vbuffer->size, 0, data);
	}
	RESULT Buffer::UnMap()
	{
		auto vbuffer = (vBuffer*)this;
		if (vbuffer->vma_ID)
		{
			vmaUnmapMemory((device.retrieve_as<vDevice>())->allocator, vbuffer->vma_ID);
			return 0;
		}
		vkUnmapMemory((VkDevice)(device.retrieve_as<vDevice>())->ID, (VkDeviceMemory)vbuffer->heap->ID);
		return 0;
	}
}

