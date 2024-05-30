#pragma once
#include "Core.h"
#include <atomic>
#include <cstdint>
namespace RHI
{
	class RHI_API Object
	{
	protected:
		std::atomic<uint32_t>* refCnt;
		Object() { refCnt = new std::atomic<uint32_t>(); *refCnt = 1; }
		virtual int32_t GetType() {return 0;} 
	public:
		void SetName(const char* name);
		int Hold();
		int Release();
		int GetRefCount();
		Internal_ID ID = nullptr;
		virtual void Destroy() {};
		void* device = nullptr;
	};
}