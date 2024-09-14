#include "pch.h"
#include "VulkanSpecific.h"
#include "../Texture.h"
#include "volk.h"
namespace RHI
{
	ezr::result<void*, MappingError> Texture::Map()
	{
		auto vtex = reinterpret_cast<vTexture*>(this);
		void* data;
		if (vtex->vma_ID)
		{
			if (const auto res = vmaMapMemory((device.retrieve_as_forced<vDevice>())->allocator, vtex->vma_ID, &data); res < VK_SUCCESS)
				return ezr::err(MappingError::Unknown);
			return data;
		}
		if(const auto res = vkMapMemory(static_cast<VkDevice>(device.retrieve_as_forced<vDevice>()->ID), static_cast<VkDeviceMemory>(vtex->heap->ID), vtex->offset, vtex->size, 0, &data); res < VK_SUCCESS)
			return ezr::err(MappingError::Unknown);
		return data;
	}
	void Texture::UnMap()
	{
		auto vtex = (vTexture*)this;
		if (vtex->vma_ID)
		{
			vmaUnmapMemory((device.retrieve_as_forced<vDevice>())->allocator, vtex->vma_ID);
            return;
		}
		vkUnmapMemory(static_cast<VkDevice>(device.retrieve_as_forced<vDevice>()->ID), static_cast<VkDeviceMemory>(vtex->heap->ID));
	}
}

