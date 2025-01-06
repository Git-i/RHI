#include "pch.h"
#include "../Object.h"
#include "VulkanSpecific.h"
#include <cstring>

namespace RHI {
	uint32_t Object::Hold()
	{
		refCnt += 1;
		return refCnt;
	}
	uint32_t Object::Release()
	{
		refCnt -= 1;
		if (refCnt <= 0) {
			delete name;
			name = nullptr;
			delete this;
			return 0;
		}
		return refCnt;
	}
	uint32_t Object::GetRefCount()
	{
		return refCnt;
	}

	DeviceChild::~DeviceChild()
	{
		if(device.IsValid())
			device.retrieve_as_forced<vDevice>()->objects.erase(this);
	}

	void Object::SetName(const char* obj_name)
	{
		VkDebugUtilsObjectNameInfoEXT name_info{};
		name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		name = new char[strlen(obj_name)+1];
		strcpy(name, obj_name);
		name_info.pObjectName = name;
		name_info.objectHandle = (uint64_t)ID;
		name_info.objectType = (VkObjectType)GetType();
		VkDevice d = nullptr;
		if (name_info.objectType == VK_OBJECT_TYPE_DEVICE) d = static_cast<VkDevice>(ID);
		else if(auto child = dynamic_cast<DeviceChild*>(this)) d = static_cast<VkDevice>(child->device.retrieve_as_forced<Device>()->ID);
		else return;
		vkSetDebugUtilsObjectNameEXT(d, &name_info);
	}
}
