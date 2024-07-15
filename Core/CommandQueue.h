#pragma once
#include "Core.h"
#include "Object.h"
#include "FormatsAndTypes.h"
#include "Fence.h"
#include "Ptr.h"
namespace RHI
{
	class RHI_API CommandQueue : public Object
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(CommandQueue);
	public:
		static CommandQueue* FromNativeHandle(Internal_ID id);
		RESULT WaitForFence(Weak<Fence>, std::uint64_t val);
		RESULT SignalFence(Weak<Fence> fence, std::uint64_t val);
		RESULT ExecuteCommandLists(const Internal_ID* lists, std::uint32_t count);

	};
	struct CommandQueueDesc
	{
		CommandListType commandListType;
		float Priority;
		int _unused = 0;
	};
}
