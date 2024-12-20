#include "FormatsAndTypes.h"
#include "../PhysicalDevice.h"
#include "VulkanSpecific.h"
#include <cstdint>
#include <unordered_map>
#include <vulkan/vulkan_core.h>
namespace RHI
{
	Ptr<PhysicalDevice> PhysicalDevice::FromNativeHandle(Internal_ID id)
	{
		Ptr<PhysicalDevice> dev(new vPhysicalDevice);
		dev->ID = id;
		dev->Hold();
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
			default: return Unknown;
		}
	}
	FormatSupport ToRHIType(VkFormatFeatureFlags vk_props)
	{
		using enum FormatSupport;
		FormatSupport support = None;
		if(vk_props) support |= Supported; else return support;
		if(vk_props & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) support |= VertexBuffer;
		if(vk_props & VK_FORMAT_FEATURE_BLIT_SRC_BIT) support |= BlitSrc;
		if(vk_props & VK_FORMAT_FEATURE_BLIT_DST_BIT) support |= BlitDst;
		if(vk_props & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) support |= ColorAttachment;
		if(vk_props & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT) support |= BlendableColorAttachment;
		if(vk_props & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) support |= DepthStencilAttachment;
		if(vk_props & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) support |= SampledImage;
		if(vk_props & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) support |= StorageImage;
		return support;
	}
	FormatSupport PhysicalDevice::GetFormatSupportInfo(RHI::Format format, TextureTilingMode t)
	{
		auto vk = FormatConv(format);
		if(vk == VK_FORMAT_UNDEFINED) return FormatSupport::None;
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties((VkPhysicalDevice)ID, vk, &props);
		return ToRHIType(t == TextureTilingMode::Linear ? props.linearTilingFeatures : props.optimalTilingFeatures);
	}
	FormatSupport PhysicalDevice::GetBufferFormatSupportInfo(RHI::Format format)
	{
		auto vk = FormatConv(format);
		if(vk == VK_FORMAT_UNDEFINED) return FormatSupport::None;
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties((VkPhysicalDevice)ID, vk, &props);
		return ToRHIType(props.bufferFeatures);
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
	QueueInfo PhysicalDevice::GetQueueInfo()
	{
		auto[family, count] = findQueueFamilyIndices(Weak(this));
		std::unordered_map<uint32_t, uint32_t> family_to_count_index;
		QueueInfo info;
		uint32_t next_count_index = 0;
		if(family.flags & HasGraphics)
		{
			info.graphicsSupported = true;
			info.gfxCountIndex = next_count_index;
			info.counts[next_count_index] = count[family.graphicsIndex];
			family_to_count_index[family.graphicsIndex] = next_count_index++;
			
		}
		if(family.flags & HasCompute)
		{
			info.computeSupported = true;
			uint32_t ci;
			if(family_to_count_index.contains(family.computeIndex)) ci = family_to_count_index[family.computeIndex];
			else ci = next_count_index++;
			info.counts[ci] = count[family.computeIndex];
			info.cmpCountIndex = ci;
			family_to_count_index[family.computeIndex] = ci;
		}
		if(family.flags & HasCopy)
		{
			info.copySupported = true;
			uint32_t ci;
			if(family_to_count_index.contains(family.copyIndex)) ci = family_to_count_index[family.copyIndex];
			else ci = next_count_index++;
			info.counts[ci] = count[family.copyIndex];
			info.copyCountIndex = ci;
		}
		return info;
	}
}
