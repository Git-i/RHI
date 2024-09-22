#include "pch.h"
#include "VulkanSpecific.h"
#include "../Buffer.h"
#include "volk.h"
namespace RHI
{
	ezr::result<void*, MappingError> Buffer::Map()
	{
		auto vbuffer = reinterpret_cast<vBuffer*>(this);
		if(vbuffer->mapped_data){}
		else if (vbuffer->is_auto())
		{
			if(const auto res = vmaMapMemory((device.retrieve_as_forced<vDevice>())->allocator,
				vbuffer->vma_allocation(), reinterpret_cast<void**>(&vbuffer->mapped_data)); res < VK_SUCCESS)
				return ezr::err(MappingError::Unknown);
		}
		else
		{
			if(const auto res = vkMapMemory(static_cast<VkDevice>(device->ID), static_cast<VkDeviceMemory>(vbuffer->placed_data().heap->ID),
				vbuffer->placed_data().offset, vbuffer->placed_data().size, 0, reinterpret_cast<void**>(&vbuffer->mapped_data)); res < VK_SUCCESS)
				return ezr::err(MappingError::Unknown);
		}
		++vbuffer->map_ref;
		return vbuffer->mapped_data;
	}
	void Buffer::UnMap()
	{
		auto vbuffer = reinterpret_cast<vBuffer*>(this);
		if(!vbuffer->mapped_data) return;
		--vbuffer->map_ref;
		if(vbuffer->map_ref > 0) return;

		if (vbuffer->is_auto()) vmaUnmapMemory((device.retrieve_as_forced<vDevice>())->allocator, vbuffer->vma_allocation());
		else vkUnmapMemory(static_cast<VkDevice>(device->ID), static_cast<VkDeviceMemory>(vbuffer->placed_data().heap->ID));

		vbuffer->mapped_data = nullptr;
	}
}

