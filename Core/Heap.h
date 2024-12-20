#pragma once
#include "Object.h"
namespace RHI
{
	class RHI_API Heap : public DeviceChild
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(Heap);
	};
	enum class HeapType
	{
		Default,
		Upload,
		ReadBack,
		Custom
	};
	enum class CPUPageProperty
	{
		WriteCombined = 0,
		WriteCached = 1,
		WriteBack = 1,
		Any = 3,
		NonVisible = 2
	};
	enum class MemoryLevel
	{
		DedicatedRAM,
		SharedRAM,
		Unknown
	};
	struct RHI_API HeapProperties
	{
		HeapType type;
		CPUPageProperty pageProperty;
		MemoryLevel memoryLevel;
		HeapType FallbackType;
	};
	struct RHI_API HeapDesc
	{
		std::uint64_t size;
		HeapProperties props;
	};
	struct RHI_API MemoryRequirements
	{
		std::uint64_t size;
		std::uint64_t alignment;
		uint32_t memoryTypeBits;
	};
}
