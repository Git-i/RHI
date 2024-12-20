#pragma once
#include "Object.h"
#include "FormatsAndTypes.h"
#include "Texture.h"
namespace RHI
{
	enum class TextureViewType
	{
		Texture1D,Texture2D,Texture3D,TextureCube,
		Texture1DArray, Texture2DArray, TextureCubeArray
	};
	struct TextureViewDesc
	{
		TextureViewType type;
		Format format;
		Weak<Texture> texture;
		SubResourceRange range;
		//TODO 4 component mapping
	};
	class TextureView : public DeviceChild
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(TextureView);
	public:
	   Ptr<Texture> GetTexture();
	};
}
