#include "pch.h"
#include "../Object.h"
#include "VulkanSpecific.h"
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
			return 0;
		}
		return *refCnt;
	}
	int Object::GetRefCount()
	{
		return *refCnt;
	}
	void Object::SetName(const char* name)
	{
		VkDebugUtilsObjectNameInfoEXT name_info{};
		name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		name_info.pObjectName = name;
		name_info.objectHandle = (uint64_t)ID;
		name_info.objectType = (VkObjectType)GetType();
		vkSetDebugUtilsObjectNameEXT( (VkDevice)((RHI::Device*)device)->ID, &name_info);
	}
}