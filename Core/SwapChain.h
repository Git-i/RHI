#pragma once
#include "Object.h"
#include "FormatsAndTypes.h"
#include "Surface.h"
#include "Texture.h"
namespace RHI
{
	class RHI_API SwapChain : public Object
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(SwapChain);
	public:
		RESULT Present(std::uint32_t imgIndex);
		RESULT AcquireImage(std::uint32_t imgIndex);

	};
	struct RHI_API SwapChainDesc
	{
		Surface OutputSurface;
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

		SwapChainDesc(Default_t);
		SwapChainDesc(Zero_t);
		SwapChainDesc();
	};
}
