#include "pch.h"
#include "VulkanSpecific.h"
#include "../Texture.h"
#include "volk.h"
namespace RHI
{
	void Texture::Map(void** data)
	{
		auto vtex = (vTexture*)this;
		if (vtex->vma_ID)
		{
			vmaMapMemory((device.retrieve_as_forced<vDevice>())->allocator, vtex->vma_ID, data);
            return;
		}
		vkMapMemory((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkDeviceMemory)vtex->heap->ID, vtex->offset, vtex->size, 0, data);
	}
	void Texture::UnMap()
	{
		auto vtex = (vTexture*)this;
		if (vtex->vma_ID)
		{
			vmaUnmapMemory((device.retrieve_as_forced<vDevice>())->allocator, vtex->vma_ID);
            return;
		}
		vkUnmapMemory((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkDeviceMemory)vtex->heap->ID);
	}
}

