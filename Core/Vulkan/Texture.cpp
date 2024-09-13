#include "pch.h"
#include "VulkanSpecific.h"
#include "../Texture.h"
#include "volk.h"
namespace RHI
{
	ezr::result<void*, MappingError> Texture::Map()
	{
		auto vtex = (vTexture*)this;
		void* data;
		if (vtex->vma_ID)
		{
			auto res = vmaMapMemory((device.retrieve_as_forced<vDevice>())->allocator, vtex->vma_ID, &data);
			if (res < VK_SUCCESS) return ezr::err(MappingError::Unknown);
			return data;
		}
		vkMapMemory((VkDevice)(device.retrieve_as_forced<vDevice>())->ID, (VkDeviceMemory)vtex->heap->ID, vtex->offset, vtex->size, 0, &data);
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

