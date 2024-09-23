#pragma once
#include "Object.h"
#include "FormatsAndTypes.h"
namespace RHI
{
	class RHI_API Buffer : public Object
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(Buffer);
	public:
		[[nodiscard]] ezr::result<void*, MappingError> Map();
		void UnMap();
	};
	struct RHI_API BufferDesc
	{
		std::uint32_t size;
		BufferUsage usage;
	};
}
