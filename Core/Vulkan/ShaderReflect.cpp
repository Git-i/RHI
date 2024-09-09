#include "FormatsAndTypes.h"
#include "Instance.h"
#include "Ptr.h"
#include "RootSignature.h"
#include "pch.h"
#include "VulkanSpecific.h"
#include "../ShaderReflect.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>
#include "result.hpp"
#include "spirv_reflect.h"
#include "volk.h"

namespace RHI
{
	static constexpr size_t operator""_sz(unsigned long long int x)
	{
		return static_cast<size_t>(x);
	}
	creation_result<ShaderReflection> ShaderReflection::CreateFromFile(std::string_view filename)
	{
		Ptr<vShaderReflection> vReflection(new vShaderReflection);
		if(!std::filesystem::exists(filename)) return ezr::err(CreationError::FileNotFound);
		std::ifstream file(std::string(filename), std::ios::binary);
		uint32_t spvSize = 0;
		file.read((char*)&spvSize, 4);
		
		std::span bytes = std::as_writable_bytes(std::span{&spvSize, 1});
		if(std::endian::native != std::endian::little) std::reverse(bytes.begin(), bytes.end());
		if(spvSize == 0) return ezr::err(CreationError::IncompatibleShader);
		
		std::vector<char> buffer(spvSize);
		file.read(buffer.data(), spvSize);

		auto module = new SpvReflectShaderModule;
		vReflection->ID = module;
		SpvReflectResult result = spvReflectCreateShaderModule(buffer.size(), buffer.data(), module);
		if(result != SPV_REFLECT_RESULT_SUCCESS) return ezr::err(RHI::CreationError::Unknown);
		//TODO
		return ezr::ok(vReflection.transform<ShaderReflection>());
	}
	creation_result<ShaderReflection> ShaderReflection::CreateFromMemory(std::string_view buffer)
	{
		Ptr<vShaderReflection> vReflection(new vShaderReflection);
		auto module = new SpvReflectShaderModule;
		vReflection->ID = module;
		SpvReflectResult result = spvReflectCreateShaderModule(buffer.size(), buffer.data(), module);
		if(result != SPV_REFLECT_RESULT_SUCCESS) return ezr::err(RHI::CreationError::Unknown);
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
			set[i].reflection = make_ptr(this);
		}
	}
	void ShaderReflection::GetDescriptorSet(uint32_t set_index, SRDescriptorSet* set)
	{
		SpvReflectResult res;
		auto SPVset = spvReflectGetDescriptorSet((SpvReflectShaderModule*)ID, set_index, &res);
		set->bindingCount = SPVset->binding_count;
		set->setIndex = SPVset->set;
		set->reflection = make_ptr(this);
	}
	static DescriptorClass convertTOSRType(SpvReflectResourceType type)
	{
		switch (type)
		{
		case SPV_REFLECT_RESOURCE_FLAG_SAMPLER: return DescriptorClass::Sampler;
		case SPV_REFLECT_RESOURCE_FLAG_CBV: return DescriptorClass::CBV;
		case SPV_REFLECT_RESOURCE_FLAG_SRV: return DescriptorClass::SRV;
		case SPV_REFLECT_RESOURCE_FLAG_UAV: return DescriptorClass::UAV;
		default: return DescriptorClass(-1);
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
		default: return DescriptorType(-1);
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
	static bool DifferentStages(std::span<Ptr<ShaderReflection>> refl)
	{
		RHI::ShaderStage stage = RHI::ShaderStage::None;
		for(auto ptr : refl)
		{
			if((stage & ptr->GetStage()) != RHI::ShaderStage::None) return false;
			stage |= ptr->GetStage();
		}
		return true;
	}
	std::vector<SRDescriptorSet> GetSetsForIndex(std::vector<std::vector<SRDescriptorSet>>& sets,
		std::span<RHI::Ptr<ShaderReflection>> refl,
		uint32_t index)
	{
		std::vector<SRDescriptorSet> retVal;
		auto sz = refl.size();
		for(auto i : std::views::iota(0_sz, sz))
		{
			for(auto& set: sets[i])
			{
				if(set.setIndex == index)
				{
					retVal.push_back(set);
				}
			}
		}
		return retVal;
	}
	std::vector<std::pair<SRDescriptorBinding, ShaderStage>> GetBindingsForIndex(std::vector<SRDescriptorSet>& sets, uint32_t index)
	{
		std::vector<std::pair<SRDescriptorBinding, ShaderStage>> retVal;
		auto sz = sets.size();
		for(auto i : std::views::iota(0_sz, sz))
		{
			std::vector<SRDescriptorBinding> bindings(sets[i].bindingCount);
			sets[i].reflection->GetDescriptorSetBindings(&sets[i], bindings.data());
			for(auto& binding : bindings)
			{
				if(binding.bindingSlot == index)
				{
					retVal.push_back(std::pair{binding, sets[i].reflection->GetStage()});
				}
			}
		}
		return retVal;
	}
	/*
		Push Constants must be on set 0
	*/
	auto ShaderReflection::FillRootSignatureDesc(std::span<RHI::Ptr<ShaderReflection>> refl, std::span<const uint32_t> dynamic_sets, std::optional<uint32_t> push_block) -> std::tuple<
			RootSignatureDesc,
			std::vector<RootParameterDesc>,
			std::vector<std::vector<DescriptorRange>>>
	{
		using enum RootParameterType;
		assert(DifferentStages(refl) && "Cannot Have Two shaders for the same stage");
		RootSignatureDesc rsDesc;
		std::vector<RootParameterDesc> rpDescs;
		std::vector<std::vector<DescriptorRange>> ranges;
		std::vector<std::vector<SRDescriptorSet>> sets;
		for(auto ptr : refl)
		{
			std::vector<SRPushConstantBlock> blks(ptr->GetNumPushConstantBlocks());
			ptr->GetAllPushConstantBlocks(blks.data());
			for(auto& blk : blks)
			{
				assert(push_block.has_value());
				auto& desc = rpDescs.emplace_back();
				desc.type = PushConstant;
				desc.pushConstant.numConstants = blk.num_constants;
				desc.pushConstant.bindingIndex = push_block.value();
				desc.pushConstant.offset = 0;
				desc.pushConstant.stage = ptr->GetStage();
			}
			sets.emplace_back(ptr->GetNumDescriptorSets());
			ptr->GetAllDescriptorSets(sets.back().data());
		}
		uint32_t max_set_index = 0;
		for(auto& set: sets)
		{
			max_set_index = std::max(max_set_index, [&]()->uint32_t{
				uint32_t max = 0;
				for(auto& srSet: set)
				{
					max = std::max(max, srSet.setIndex);
				}
				return max;
			}());
		}
		max_set_index++;
		for(uint32_t i : std::views::iota(0U, max_set_index))
		{
			std::vector current_sets = GetSetsForIndex(sets, refl, i);
			if(current_sets.size() == 0) continue;
			auto& param = rpDescs.emplace_back();
			if(std::find(dynamic_sets.begin(), dynamic_sets.end(), i) != dynamic_sets.end())
			{
			    assert(current_sets.size());
				SRDescriptorBinding bnd;
				current_sets[0].reflection->GetDescriptorSetBindings(&current_sets[0], &bnd);
				for(auto& set : current_sets)
				{
				    assert(set.bindingCount == 1 && "Dynamic Descriptors Have only One Binding");
					SRDescriptorBinding bnd2;
					set.reflection->GetDescriptorSetBindings(&set, &bnd2);
					assert(bnd2.bindingSlot == bnd.bindingSlot && bnd.resourceType == bnd2.resourceType);
				}

			    param.type = RootParameterType::DynamicDescriptor;
				param.dynamicDescriptor.type = DynamicOf(bnd.resourceType);
				param.dynamicDescriptor.setIndex = i;
				param.dynamicDescriptor.stage = ShaderStage::None;
				for(auto& set : current_sets) param.dynamicDescriptor.stage |= set.reflection->GetStage();
				continue;
			}
			param.type = RootParameterType::DescriptorTable;
			param.descriptorTable.setIndex = i;
			auto& curr_ranges = ranges.emplace_back();
			//fill ranges
			uint32_t max_binding_c = 0;
			for(auto& set: current_sets)
			{
			    max_binding_c = std::max(max_binding_c, set.bindingCount);
			}
			for(uint32_t j : std::views::iota(0U, max_binding_c))
			{
				std::vector bindings = GetBindingsForIndex(current_sets, j);
				if(bindings.size() == 0) continue;
				auto& range = curr_ranges.emplace_back();
				range.BaseShaderRegister = bindings[0].first.bindingSlot;
				range.numDescriptors = bindings[0].first.count;
				range.type = bindings[0].first.resourceType;
				range.stage = ShaderStage::None;
				for(auto& bnd : bindings) range.stage |= bnd.second;
			}
			param.descriptorTable.numDescriptorRanges = curr_ranges.size();
			param.descriptorTable.ranges = curr_ranges.data();
		}
		
		rsDesc.numRootParameters = rpDescs.size();
		rsDesc.rootParameters = rpDescs.data();
		return std::tuple{rsDesc, std::move(rpDescs), std::move(ranges)};
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
}
