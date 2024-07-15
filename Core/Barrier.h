#pragma once
#include "Core.h"
#include "FormatsAndTypes.h"
#include "Texture.h"
#include "Buffer.h"
namespace RHI
{
	enum class QueueFamily
	{
		Graphics, Compute, Copy, Ignored
	};
	struct RHI_API TextureMemoryBarrier
	{
		ResourceAcessFlags         AccessFlagsBefore;
		ResourceAcessFlags         AccessFlagsAfter;
		ResourceLayout              oldLayout;
		ResourceLayout             newLayout;
		Weak<Texture>              texture;
		QueueFamily				   previousQueue;
		QueueFamily				   nextQueue;
		SubResourceRange   subresourceRange;
	};
	struct RHI_API BufferMemoryBarrier
	{
		ResourceAcessFlags         AccessFlagsBefore;
		ResourceAcessFlags         AccessFlagsAfter;
		QueueFamily				   previousQueue;
		QueueFamily				   nextQueue;
		Weak<Buffer>                   buffer;
		uint32_t                   offset;
		uint32_t                   size;
	};
}
