#include "RootSignature.h"
#include "pch.h"
#include "VulkanSpecific.h"
#include "../ShaderReflect.h"
#include <fstream>
#include <vector>
#include "spirv_reflect.h"
#include "volk.h"
namespace RHI
{
	creation_result<ShaderReflection> ShaderReflection::CreateFromFile(const char* filename)
	{
		Ptr<vShaderReflection> vReflection(new vShaderReflection);
		auto module = new SpvReflectShaderModule;
		vReflection->ID = module;
		char actual_name[1024];
		strcpy(actual_name, filename);
		strcat(actual_name, ".spv");
		std::ifstream file(actual_name, std::ios::ate | std::ios::binary);
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		SpvReflectResult result = spvReflectCreateShaderModule(buffer.size(), buffer.data(), module);
		//TODO
		return ezr::ok(vReflection.transform<ShaderReflection>());
	}
	creation_result<ShaderReflection> ShaderReflection::CreateFromMemory(const char* buffer, uint32_t size)
	{
		Ptr<vShaderReflection> vReflection(new vShaderReflection);
		auto module = new SpvReflectShaderModule;
		vReflection->ID = module;
		SpvReflectResult result = spvReflectCreateShaderModule(size, buffer, module);
		
		return ezr::ok(vReflection.transform<ShaderReflection>());
	}
	uint32_t ShaderReflection::GetNumDescriptorSets()
	{
		uint32_t count;
		spvReflectEnumerateDescriptorSets((SpvReflectShaderModule*)ID, &count, NULL);
		return count;
	}
	void ShaderReflection::GetAllDescriptorSets(SRDescriptorSet* set)
	{
		uint32_t count = GetNumDescriptorSets();
		std::vector<SpvReflectDescriptorSet*> sets(count);
		spvReflectEnumerateDescriptorSets((SpvReflectShaderModule*)ID, &count, sets.data());
		for (uint32_t i = 0; i < count ; i++)
		{
			set[i].setIndex = sets[i]->set;
			set[i].bindingCount = sets[i]->binding_count;
		}
	}
	void ShaderReflection::GetDescriptorSet(uint32_t set_index, SRDescriptorSet* set)
	{
		SpvReflectResult res;
		auto SPVset = spvReflectGetDescriptorSet((SpvReflectShaderModule*)ID, set_index, &res);
		set->bindingCount = SPVset->binding_count;
		set->setIndex = SPVset->set;
	}
	static DescriptorClass convertTOSRType(SpvReflectResourceType type)
	{
		switch (type)
		{
		case SPV_REFLECT_RESOURCE_FLAG_SAMPLER: return DescriptorClass::Sampler;
			break;
		case SPV_REFLECT_RESOURCE_FLAG_CBV: return DescriptorClass::CBV;
			break;
		case SPV_REFLECT_RESOURCE_FLAG_SRV: return DescriptorClass::SRV;
			break;
		case SPV_REFLECT_RESOURCE_FLAG_UAV: return DescriptorClass::UAV;
			break;
		default:
			break;
		}
	}
	static DescriptorType convertTOSRType(SpvReflectDescriptorType type, SpvReflectResourceType rtype)
	{
		switch (type)
		{
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return DescriptorType::Sampler;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return DescriptorType::SampledTexture;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return DescriptorType::ConstantBuffer;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		{
			return rtype == SPV_REFLECT_RESOURCE_FLAG_UAV ? DescriptorType::CSBuffer : DescriptorType::StructuredBuffer;
			break;
		}
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return DescriptorType::ConstantBufferDynamic;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		{
			return rtype == SPV_REFLECT_RESOURCE_FLAG_UAV ? DescriptorType::CSBufferDynamic : DescriptorType::StructuredBufferDynamic;
			break;
		}
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return DescriptorType::CSTexture;
			break;
		default:
			break;
		}
	}
	void ShaderReflection::GetDescriptorSetBindings(SRDescriptorSet* set, SRDescriptorBinding* bindings)
	{
		SpvReflectResult res;
		auto SPVset = spvReflectGetDescriptorSet((SpvReflectShaderModule*)ID, set->setIndex, &res);
		for (uint32_t i = 0; i < SPVset->binding_count; i++)
		{
			bindings[i].resourceClass = convertTOSRType(SPVset->bindings[i]->resource_type);
			bindings[i].resourceType = convertTOSRType(SPVset->bindings[i]->descriptor_type, SPVset->bindings[i]->resource_type);
			bindings[i].bindingSlot = SPVset->bindings[i]->binding;
			bindings[i].count = SPVset->bindings[i]->count;
			bindings[i].setIndex = SPVset->bindings[i]->set;
			bindings[i].name = SPVset->bindings[i]->name;
		}
	}
	auto ShaderReflection::FillRootSignatureDesc(RHI::ShaderStage stage) -> std::tuple<
			RootSignatureDesc,
			std::vector<RootParameterDesc>,
			std::vector<std::vector<DescriptorRange>>>
	{
		RootSignatureDesc rsDesc;
		std::vector<RootParameterDesc> rootParams;
		std::vector<std::vector<DescriptorRange>> ranges_vec;
		uint32_t numSets = GetNumDescriptorSets();
		std::vector<RHI::SRDescriptorSet> sets(numSets);
		GetAllDescriptorSets(sets.data());
		
		for (uint32_t i = 0; i < numSets; i++)
		{
			auto& ranges = ranges_vec.emplace_back(ranges_vec);
			RHI::RootParameterDesc desc;

			desc.type = RHI::RootParameterType::DescriptorTable;
			desc.descriptorTable.setIndex = sets[i].setIndex;
			desc.descriptorTable.numDescriptorRanges = sets[i].bindingCount;
			std::vector<RHI::SRDescriptorBinding> bindings(sets[i].bindingCount);
			GetDescriptorSetBindings(&sets[i], bindings.data());
			//fill out all the ranges (descriptors)
			for (uint32_t j = 0; j < sets[i].bindingCount; j++)
			{
				RHI::DescriptorRange range;
				range.BaseShaderRegister = bindings[j].bindingSlot;
				range.numDescriptors = bindings[j].count;
				range.type = bindings[j].resourceType;
				range.stage = stage;
				ranges.push_back(range);
			}
			desc.descriptorTable.ranges = ranges.data();
			rootParams.push_back(desc);
		}
		rsDesc.numRootParameters = rootParams.size();
		rsDesc.rootParameters = rootParams.data();
		return {rsDesc, std::move(rootParams), std::move(ranges_vec)};
	}
}