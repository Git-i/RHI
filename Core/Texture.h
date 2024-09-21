#pragma once
#include "Object.h"
#include "FormatsAndTypes.h"
namespace RHI
{
	enum class MappingError{ Unknown };
	class RHI_API  Texture : public Object
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(Texture);
	public:
		/**
		 * Multiple calls to @c Map can be made on the same resource, but only one resource per heap must be mapped when
		 * using placed resources. To write to multiple placed resources in the same heap you should map a region of the heap
		* via @c Heap::Map then calling @c MapFromHeapPtr
		*/
		ezr::result<void*, MappingError> Map(Aspect, uint32_t mip, uint32_t layer);
		void* MapFromHeapPtr(void* heap_ptr, uint32_t offset, Aspect, uint32_t mip, uint32_t layer);
		void UnMap(Aspect, uint32_t mip, uint32_t layer);
	};

	
	enum class TextureUsage
	{
		None = 0,
		CopyDst = 1 << 0,
		SampledImage = 1 << 1,
		ColorAttachment = 1 << 2,
		DepthStencilAttachment = 1 << 3,
		CubeMap = 1 << 4,
		CopySrc = 1 << 5,
		StorageImage = 1 << 6
	};
	DEFINE_ENUM_FLAG_OPERATORS(TextureUsage);
	struct RHI_API TextureDesc
	{
		TextureType type;
		std::uint32_t width;
		std::uint32_t height;
		std::uint32_t depthOrArraySize;
		Format format;
		std::uint32_t mipLevels;
		std::uint32_t sampleCount;
		TextureTilingMode mode;
		ClearValue* optimizedClearValue = nullptr;
		TextureUsage usage;
		ResourceLayout layout = ResourceLayout::UNDEFINED;
	};
	struct RHI_API RenderTargetViewDesc
	{
		bool TextureArray;
		uint32_t arraySlice;
		RHI::Format format;
		uint32_t textureMipSlice;
	};
	struct RHI_API DepthStencilViewDesc
	{
		bool TextureArray;
		uint32_t arraySlice;
		RHI::Format format;
		uint32_t textureMipSlice;
	};
	enum class AddressMode
	{
		Border, Clamp, Mirror,Wrap
	};
	enum class Filter
	{
		Linear, Nearest
	};
	struct RHI_API SamplerDesc
	{
		AddressMode AddressU;
		AddressMode AddressV;
		AddressMode AddressW;
		bool anisotropyEnable;
		bool compareEnable;
		ComparisonFunc compareFunc;
		Filter minFilter;
		Filter magFilter;
		Filter mipFilter;
		uint32_t maxAnisotropy;
		float minLOD;
		float maxLOD;
		float mipLODBias;
	};
}
