#pragma once
#include "Fence.h"
#include "Object.h"
#include "FormatsAndTypes.h"
#include "Surface.h"
#include "Texture.h"
#include <cstdint>
namespace RHI
{
	enum class SwapChainError
	{
		None,
		OutOfDate,
		OutOfMemory,
		DeviceLost,
		SubOptimal,
		Unknown
	};
	class RHI_API SwapChain : public DeviceChild
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(SwapChain);
	public:
		[[nodiscard]] SwapChainError Present(RHI::Weak<RHI::Fence> waitBeforePresent, uint64_t waitVal);
		uint32_t GetImageIndex();
	};
	struct RHI_API SwapChainDesc
	{
		Weak<Surface> OutputSurface;
		unsigned int Width;
		unsigned int Height;
		Rational RefreshRate;
		Format SwapChainFormat;
		unsigned int SampleCount;
		unsigned int SampleQuality;
		//DXGI_USAGE BufferUsage;
		unsigned int BufferCount;
		bool Windowed;
		//DXGI_SWAP_EFFECT SwapEffect;
		uint32_t Flags;
	};
}
