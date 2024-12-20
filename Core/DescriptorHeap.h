#pragma once
#include "Buffer.h"
#include "FormatsAndTypes.h"
#include "Object.h"
#include "TextureView.h"
namespace RHI
{
	class RHI_API DescriptorHeap : public DeviceChild
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(DescriptorHeap);
	public:
		CPU_HANDLE GetCpuHandle();
	};

	class RHI_API DescriptorSetLayout : public DeviceChild
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(DescriptorSetLayout);
	};

	class RHI_API DescriptorSet : public DeviceChild
	{
	protected:
	    DECL_CLASS_CONSTRUCTORS(DescriptorSet);
	public:
	    Ptr<DescriptorSetLayout> GetLayout();
	    Ptr<DescriptorHeap> GetHeap();
	};

	class RHI_API DynamicDescriptor : public DeviceChild
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(DynamicDescriptor);
	public:
	    Ptr<DescriptorHeap> GetHeap();
	};

	struct DescriptorBufferInfo
	{
		Weak<Buffer> buffer;
		std::uint32_t offset;
		std::uint32_t range;
	};
	enum class ShaderResourceViewDimension
	{
		Texture1D, Texture1DArray, Texture2D, Texture2DArray, Texture3D, Texture3DArray
	};
	struct DescriptorTextureInfo
	{
		Weak<TextureView> texture;
	};
	struct DescriptorSamplerInfo
	{
		CPU_HANDLE heapHandle;
	};
	struct RHI_API DescriptorSetUpdateDesc
	{
		std::uint32_t binding;
		std::uint32_t arrayIndex;
		std::uint32_t numDescriptors;
		DescriptorType type;
		union
		{
			DescriptorBufferInfo* bufferInfos;
			DescriptorTextureInfo* textureInfos;
			DescriptorSamplerInfo* samplerInfos;
		};
	};
	struct RHI_API PoolSize
	{
		DescriptorType type;
		std::uint32_t numDescriptors;
	};
	struct RHI_API DescriptorHeapDesc
	{
		std::uint32_t maxDescriptorSets;
		std::uint32_t numPoolSizes;
		PoolSize* poolSizes;
	};
}
