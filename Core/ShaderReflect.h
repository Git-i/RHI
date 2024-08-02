#pragma once
#include "Object.h"
#include "FormatsAndTypes.h"
#include "RootSignature.h"
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
	struct SRDescriptorSet
	{
		uint32_t setIndex;
		uint32_t bindingCount;
	};
	class RHI_API ShaderReflection : public Object
	{
	protected:
		DECL_CLASS_CONSTRUCTORS(ShaderReflection)
	public:
		static creation_result<ShaderReflection> CreateFromFile(const char* filename);
		static creation_result<ShaderReflection> CreateFromMemory(const char* buffer,uint32_t size);
		auto FillRootSignatureDesc(RHI::ShaderStage stage) -> std::tuple<
			RootSignatureDesc,
			std::vector<RootParameterDesc>,
			std::vector<std::vector<DescriptorRange>>>;
		RootSignatureDesc Concatenate(std::initializer_list<RootSignatureDesc> list);
		uint32_t GetNumDescriptorSets();
		void GetAllDescriptorSets(SRDescriptorSet* set);
		void GetDescriptorSet(uint32_t set_index, SRDescriptorSet* set);
		// set is a pointer to a valid SRDescriptorSet, and bindings is pointer to an array of SRDescriptorBinding, with a size of set.bindingCount
		void GetDescriptorSetBindings(SRDescriptorSet* set, SRDescriptorBinding* bindings);;
	};
}
