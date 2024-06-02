#include "pch.h"
#include "../Object.h"
#include "VulkanSpecific.h"
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace RHI {
	int Object::Hold()
	{
		*refCnt += 1;
		return *refCnt;
	}
	int Object::Release()
	{
		*refCnt -= 1;
		if (*refCnt <= 0) {
			this->Destroy();
			delete name;
			return 0;
		}
		return *refCnt;
	}
	int Object::GetRefCount()
	{
		return *refCnt;
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
		vkSetDebugUtilsObjectNameEXT( (VkDevice)((RHI::Device*)device)->ID, &name_info);
	}
}