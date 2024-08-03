#pragma once
#include "DescriptorHeap.h"
#include "Object.h"
#include "FormatsAndTypes.h"
#include "RootSignature.h"
#include "result.hpp"
#include <cstdint>
#include <initializer_list>
#include <string>
namespace RHI
{
	struct SRDescriptorBinding
	{
		DescriptorClass resourceClass;
		DescriptorType resourceType;
		uint32_t bindingSlot;
		uint32_t count;
		uint32_t setIndex;
		std::string name;
	};
	struct SRPushConstantBlock
	{
		uint32_t num_constants;
		uint32_t bindingIndex;
	};
	struct SRDescriptorSet
	{
		uint32_t setIndex;
		uint32_t bindingCount;
	};
	enum class MergeError
	{
		None,
		DynamicDescriptorConflict,
		DescriptorRangeConflict
	};
	class RHI_API ShaderReflection : public Object
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(ShaderReflection)
	public:
		static creation_result<ShaderReflection> CreateFromFile(const char* filename);
		static creation_result<ShaderReflection> CreateFromMemory(const char* buffer,uint32_t size);
		auto FillRootSignatureDesc(uint32_t* dynamic_sets = nullptr, uint32_t count = 0) -> std::tuple<
			RootSignatureDesc,
			std::vector<RootParameterDesc>,
			std::vector<std::vector<DescriptorRange>>>;
		ShaderStage GetStage();
		[[deprecated("use this at your own risk, as it is not guaranteed to produce perfect results")]]
		auto Concatenate(const RootSignatureDesc& left, const RootSignatureDesc& right) -> ezr::result<std::tuple<
			RootSignatureDesc,
			std::vector<RootParameterDesc>,
			std::vector<std::vector<DescriptorRange>>>,
			MergeError>;
		uint32_t GetNumDescriptorSets();
		uint32_t GetNumPushConstantBlocks();
		void GetAllPushConstantBlocks(SRPushConstantBlock*);
		void GetAllDescriptorSets(SRDescriptorSet* set);
		void GetDescriptorSet(uint32_t set_index, SRDescriptorSet* set);
		// set is a pointer to a valid SRDescriptorSet, and bindings is pointer to an array of SRDescriptorBinding, with a size of set.bindingCount
		void GetDescriptorSetBindings(SRDescriptorSet* set, SRDescriptorBinding* bindings);;
	};
}
