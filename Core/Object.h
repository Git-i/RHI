#pragma once
#include "Core.h"
#include <atomic>
#include <cstdint>
#include "Ptr.h"
namespace RHI
{
	class RHI_API Object
	{
	protected:
		std::atomic<uint32_t> refCnt;
		Object() { refCnt = 1; }
		Object(Object const& other) = delete;
		virtual int32_t GetType() {return 0;}
	public:
		void SetName(const char* name);
		virtual ~Object(){}
		Internal_ID ID = nullptr;
		char* name = nullptr;
		uint32_t Hold();
		uint32_t Release();
		uint32_t GetRefCount();
	};
	class RHI_API DeviceChild : public Object
	{
	protected:
		friend class Object;
		friend class Device;
		friend class Instance;
		Ptr<Object> device = nullptr;
	};
}
