#include "FormatsAndTypes.h"
#include "RootSignature.h"
#include "pch.h"
#include "VulkanSpecific.h"
#include "../ShaderReflect.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <pstl/glue_algorithm_defs.h>
#include <string_view>
#include <vector>
#include "result.hpp"
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
		spvReflectEnumerateDescriptorSets((SpvReflectShaderModule*)ID, &count, nullptr);
		return count;
	}
	uint32_t ShaderReflection::GetNumPushConstantBlocks()
	{
		uint32_t count;
		spvReflectEnumeratePushConstantBlocks((SpvReflectShaderModule*)ID, &count, nullptr);
		return count;
	}
	void ShaderReflection::GetAllPushConstantBlocks(SRPushConstantBlock* block)
	{
		uint32_t count = GetNumPushConstantBlocks();
		std::vector<SpvReflectBlockVariable*> blocks(count);
		spvReflectEnumeratePushConstantBlocks((SpvReflectShaderModule*)ID, &count, blocks.data());
		for(uint32_t i = 0; i < count; i++)
		{
			block[i].num_constants = blocks[i]->size / sizeof(uint32_t);
			block[i].bindingIndex = UINT32_MAX;
		}
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
	static ShaderStage convertSpvType(SpvReflectShaderStageFlagBits stage)
	{
		ShaderStage stg = ShaderStage::None;
		if(stage & SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
			stg |= ShaderStage::Vertex;
		if(stage & SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT)
			stg |= ShaderStage::Pixel;
		if(stage & SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT)
			stg |= ShaderStage::Compute;
		if(stage & SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
			stg |= ShaderStage::Domain;
		if(stage & SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
			stg |= ShaderStage::Hull;
		if(stage & SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT)
			stg |= ShaderStage::Geometry;
		return stg;
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
	static DescriptorType DynamicOf(DescriptorType t)
	{
		switch(t)
		{
			case DescriptorType::ConstantBuffer: return DescriptorType::ConstantBufferDynamic;
			case DescriptorType::CSBuffer: return DescriptorType::CSBufferDynamic;
			case DescriptorType::StructuredBuffer: return DescriptorType::StructuredBufferDynamic;
			default: return t;
		}
	}
	ShaderStage ShaderReflection::GetStage()
	{
		return convertSpvType(((SpvReflectShaderModule*)ID)->shader_stage);
	}
	auto ShaderReflection::FillRootSignatureDesc(uint32_t* dynamic_sets, uint32_t count) -> std::tuple<
			RootSignatureDesc,
			std::vector<RootParameterDesc>,
			std::vector<std::vector<DescriptorRange>>>
	{
		auto stage = GetStage();
		RootSignatureDesc rsDesc;
		std::vector<RootParameterDesc> rootParams;
		std::vector<std::vector<DescriptorRange>> ranges_vec;
		uint32_t numSets = GetNumDescriptorSets();
		std::vector<RHI::SRDescriptorSet> sets(numSets);
		GetAllDescriptorSets(sets.data());
		
		for (uint32_t i = 0; i < numSets; i++)
		{
			RHI::RootParameterDesc desc;
			std::vector<RHI::SRDescriptorBinding> bindings(sets[i].bindingCount);
			GetDescriptorSetBindings(&sets[i], bindings.data());
			if(std::find(dynamic_sets, dynamic_sets + count, sets[i].setIndex) != dynamic_sets + count)
			{
				assert(bindings.size() == 1 && "Dynamic Descriptors only have one binding");
				desc.type = RHI::RootParameterType::DynamicDescriptor;
				desc.dynamicDescriptor.type = DynamicOf(bindings[0].resourceType);
				desc.dynamicDescriptor.setIndex = sets[i].setIndex;
				desc.dynamicDescriptor.stage = stage;
				rootParams.push_back(desc);
				continue;
			}
			auto& ranges = ranges_vec.emplace_back();

			desc.type = RHI::RootParameterType::DescriptorTable;
			desc.descriptorTable.setIndex = sets[i].setIndex;
			desc.descriptorTable.numDescriptorRanges = sets[i].bindingCount;
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
		
		uint32_t numPushConstants = GetNumPushConstantBlocks();
		std::vector<RHI::SRPushConstantBlock> blocks(numPushConstants);
		GetAllPushConstantBlocks(blocks.data());
		for(auto& block : blocks)
		{
			RHI::RootParameterDesc desc;
			desc.type = RHI::RootParameterType::PushConstant;
			desc.pushConstant.numConstants = block.num_constants;
			desc.pushConstant.stage = stage;
			desc.pushConstant.bindingIndex = block.bindingIndex;
			desc.pushConstant.offset = 0;
			rootParams.push_back(desc);
		}
		rsDesc.numRootParameters = rootParams.size();
		rsDesc.rootParameters = rootParams.data();
		return {rsDesc, std::move(rootParams), std::move(ranges_vec)};
	}
	RootParameterDesc* GetParamForSet(const RootSignatureDesc& sig, uint32_t set)
	{
		for(uint32_t i = 0; i < sig.numRootParameters; i++)
		{
			auto type = sig.rootParameters[i].type;
			switch(type)
			{
				case RootParameterType::DescriptorTable:
				{
					auto& table = sig.rootParameters[i].descriptorTable;
					if(table.setIndex == set) return &sig.rootParameters[i];
					break;
				}
				case RootParameterType::DynamicDescriptor:
				{
					auto& desc = sig.rootParameters[i].dynamicDescriptor;
					if(desc.setIndex == set) return &sig.rootParameters[i];
					break;
				}
				default: break;
			}
		}
		return nullptr;
	}
	ezr::result<std::vector<DescriptorRange>, MergeError> MergeDescriptorTable(RootParameterDesc& out, const DescriptorTable& p1, const DescriptorTable& p2)
	{
		out.type = RootParameterType::DescriptorTable;
		out.descriptorTable.numDescriptorRanges = p1.numDescriptorRanges + p2.numDescriptorRanges;
		std::vector<DescriptorRange> ranges(out.descriptorTable.numDescriptorRanges);
		//todo add checking
		ranges.insert(ranges.begin(), p1.ranges, p1.ranges + p1.numDescriptorRanges);
		ranges.insert(ranges.begin(), p2.ranges, p2.ranges + p2.numDescriptorRanges);
		out.descriptorTable.ranges = ranges.data();
		return ezr::ok(std::move(ranges));
	}
	auto ShaderReflection::Concatenate(const RootSignatureDesc& left, const RootSignatureDesc& right) -> ezr::result<std::tuple<
			RootSignatureDesc,
			std::vector<RootParameterDesc>,
			std::vector<std::vector<DescriptorRange>>>,
			MergeError>
	{
		RootSignatureDesc final;
		std::vector<RootParameterDesc> rpDesc;
		std::vector<std::vector<DescriptorRange>> ranges;
		std::vector<uint32_t> r_indices(right.numRootParameters);
		for(uint32_t i = 0; i < right.numRootParameters; i++)
		{
			r_indices[i] = i;
		}
		for(uint32_t i = 0; i < left.numRootParameters; i++)
		{
			auto& rp = left.rootParameters[i];
			if(rp.type == RootParameterType::DescriptorTable)
			{
				auto r_rp = GetParamForSet(right, rp.descriptorTable.setIndex);
				if(r_rp)
				{
					auto res = MergeDescriptorTable(rpDesc.emplace_back(), rp.descriptorTable, r_rp->descriptorTable);
					if(res.is_err())
					{
						return ezr::err(std::move(res).err());
					}
					ranges.emplace_back(std::move(res).value());
					r_indices.erase(std::remove(r_indices.begin(), r_indices.end(), r_rp - right.rootParameters));
				}
			}
			else if(rp.type == RootParameterType::DynamicDescriptor)
			{
				if(GetParamForSet(right, rp.dynamicDescriptor.setIndex))
				{
					return ezr::err(MergeError::DynamicDescriptorConflict);
				}
				rpDesc.push_back(rp);
			}
			else if(rp.type == RootParameterType::PushConstant)
			{
				rpDesc.push_back(rp);	
			}
		}
		for(auto index: r_indices)
		{
			auto& rp = right.rootParameters[index];
			rpDesc.push_back(rp);
		}
		final.numRootParameters = rpDesc.size();
		final.rootParameters = rpDesc.data();
		return ezr::ok(std::tuple{final, std::move(rpDesc), std::move(ranges)});
	}
}