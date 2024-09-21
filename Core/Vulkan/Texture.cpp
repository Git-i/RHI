#include "pch.h"
#include "VulkanSpecific.h"
#include "../Texture.h"
#include "volk.h"
namespace RHI
{
	ezr::result<void*, MappingError> Texture::Map(Aspect asp, const uint32_t mip, const uint32_t layer)
	{
		auto vtex = reinterpret_cast<vTexture*>(this);
		VkImageSubresource r{
			.aspectMask = static_cast<VkImageAspectFlags>(asp),
			.mipLevel = mip,
			.arrayLayer = layer
		};
		VkSubresourceLayout layout;
		vkGetImageSubresourceLayout(static_cast<VkDevice>(device->ID), static_cast<VkImage>(ID), &r, &layout);
		if(vtex->mapped_data)
		{
		}
		else if (vtex->vma_ID)
		{
			if (const auto res = vmaMapMemory((device.retrieve_as_forced<vDevice>())->allocator,
				vtex->vma_ID, reinterpret_cast<void**>(&vtex->mapped_data)); res < VK_SUCCESS)
				return ezr::err(MappingError::Unknown);
		}
		else
		{
			if(const auto res = vkMapMemory(static_cast<VkDevice>(device->ID), static_cast<VkDeviceMemory>(vtex->heap->ID), vtex->offset, vtex->size, 0, &std::data);
				res < VK_SUCCESS)
				return ezr::err(MappingError::Unknown);
		}
		++vtex->map_ref;
		return vtex->mapped_data + layout.offset;
	}
	void Texture::UnMap()
	{
		auto vtex = reinterpret_cast<vTexture*>(this);
		if(!vtex->mapped_data) return;
		--vtex->map_ref;
		if(vtex->map_ref > 0) return;

		if (vtex->vma_ID)
			vmaUnmapMemory((device.retrieve_as_forced<vDevice>())->allocator, vtex->vma_ID);
		else
			vkUnmapMemory(static_cast<VkDevice>(device->ID), static_cast<VkDeviceMemory>(vtex->heap->ID));

		vtex->mapped_data = nullptr;

	}
}

