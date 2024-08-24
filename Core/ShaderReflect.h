#pragma once
#include "Object.h"
#include "FormatsAndTypes.h"
#include "RootSignature.h"
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
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
	class ShaderReflection;
	struct SRDescriptorSet
	{
		uint32_t setIndex;
		uint32_t bindingCount;
		Weak<ShaderReflection> reflection;
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
		static creation_result<ShaderReflection> CreateFromFile(std::string_view filename);
		static creation_result<ShaderReflection> CreateFromMemory(std::string_view buffer);
		static auto FillRootSignatureDesc(std::span<RHI::Ptr<ShaderReflection>>, std::span<const uint32_t> dynamic_sets, std::optional<uint32_t> push_block_idx) -> std::tuple<
			RootSignatureDesc,
			std::vector<RootParameterDesc>,
			std::vector<std::vector<DescriptorRange>>>;
		ShaderStage GetStage();
		uint32_t GetNumDescriptorSets();
		uint32_t GetNumPushConstantBlocks();
		void GetAllPushConstantBlocks(SRPushConstantBlock*);
		void GetAllDescriptorSets(SRDescriptorSet* set);
		void GetDescriptorSet(uint32_t set_index, SRDescriptorSet* set);
		// set is a pointer to a valid SRDescriptorSet, and bindings is pointer to an array of SRDescriptorBinding, with a size of set.bindingCount
		void GetDescriptorSetBindings(SRDescriptorSet* set, SRDescriptorBinding* bindings);;
	};
}
