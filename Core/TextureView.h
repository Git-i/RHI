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
		//TODO 4 component mapping
		Weak<Texture> texture;
		SubResourceRange range;
	};
	class TextureView : public Object
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(TextureView);
	public:
	   Ptr<Texture> GetTexture();
	};
}
