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
		else if (vtex->is_auto())
		{
			if (const auto res = vmaMapMemory((device.retrieve_as_forced<vDevice>())->allocator,
				vtex->vma_allocation(), reinterpret_cast<void**>(&vtex->mapped_data)); res < VK_SUCCESS)
				return ezr::err(MappingError::Unknown);
		}
		else
		{
			if(const auto res =
				vkMapMemory(static_cast<VkDevice>(device->ID), static_cast<VkDeviceMemory>(vtex->placed_data().heap->ID),
					vtex->placed_data().offset, vtex->placed_data().size, 0, reinterpret_cast<void**>(&vtex->mapped_data));
				res < VK_SUCCESS)
				return ezr::err(MappingError::Unknown);
		}
		++vtex->map_ref;
		return vtex->mapped_data + layout.offset;
	}
	void Texture::UnMap(Aspect, uint32_t mip, uint32_t layer)
	{
		auto vtex = reinterpret_cast<vTexture*>(this);
		if(!vtex->mapped_data) return;
		--vtex->map_ref;
		if(vtex->map_ref > 0) return;

		if (vtex->is_auto())
			vmaUnmapMemory((device.retrieve_as_forced<vDevice>())->allocator, vtex->vma_allocation());
		else
			vkUnmapMemory(static_cast<VkDevice>(device->ID), static_cast<VkDeviceMemory>(vtex->placed_data().heap->ID));

		vtex->mapped_data = nullptr;

	}
}

