//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGSHSharedConsts.h"
#include "XUSGSphericalHarmonics.h"

#include "CSSHCubeMap.h"
#include "CSSHSum.h"
#include "CSSHNormalize.h"

using namespace std;
using namespace DirectX;
using namespace XUSG;

//--------------------------------------------------------------------------------------
// Create interfaces
//--------------------------------------------------------------------------------------
SphericalHarmonics::uptr SphericalHarmonics::MakeUnique(API api)
{
	return make_unique<SphericalHarmonics_Impl>(api);
}

SphericalHarmonics::sptr SphericalHarmonics::MakeShared(API api)
{
	return make_shared<SphericalHarmonics_Impl>(api);
}

//--------------------------------------------------------------------------------------
// Spherical-harmonics implementations
//--------------------------------------------------------------------------------------
SphericalHarmonics_Impl::SphericalHarmonics_Impl(API api) :
	m_api(api),
	m_shaderPool(nullptr),
	m_computePipelineCache(nullptr),
	m_pipelineLayoutCache(nullptr),
	m_descriptorTableCache(nullptr),
	m_pipelineLayouts(),
	m_pipelines(),
	m_uavTables(),
	m_srvTables(),
	m_baseCSIndex(0),
	m_descriptorPoolIndex(0)
{
}

SphericalHarmonics_Impl::~SphericalHarmonics_Impl()
{
}

bool SphericalHarmonics_Impl::Init(const Device* pDevice, const ShaderPool::sptr& shaderPool,
	const Compute::PipelineCache::sptr& computePipelineCache,
	const PipelineLayoutCache::sptr& pipelineLayoutCache,
	const DescriptorTableCache::sptr& descriptorTableCache,
	uint8_t baseCSIndex, uint8_t descriptorPoolIndex)
{
	// Set shader pool and states
	m_shaderPool = shaderPool;
	m_computePipelineCache = computePipelineCache;
	m_pipelineLayoutCache = pipelineLayoutCache;
	m_descriptorTableCache = descriptorTableCache;
	m_baseCSIndex = baseCSIndex;
	m_descriptorPoolIndex = descriptorPoolIndex;

	// Create resources and pipelines
	m_numSHTexels = SH_TEX_SIZE * SH_TEX_SIZE * CubeMapFaceCount;
	const auto numGroups = XUSG_DIV_UP(m_numSHTexels, SH_GROUP_SIZE);
	const auto numSumGroups = XUSG_DIV_UP(numGroups, SH_GROUP_SIZE);
	const auto maxElements = SH_MAX_ORDER * SH_MAX_ORDER * numGroups;
	const auto maxSumElements = SH_MAX_ORDER * SH_MAX_ORDER * numSumGroups;
	m_coeffSH[0] = StructuredBuffer::MakeShared(m_api);
	m_coeffSH[0]->Create(pDevice, maxElements, sizeof(float[3]),
		ResourceFlag::ALLOW_UNORDERED_ACCESS, MemoryType::DEFAULT,
		1, nullptr, 1, nullptr, MemoryFlag::NONE, L"SHCoefficients0");
	m_coeffSH[1] = StructuredBuffer::MakeShared(m_api);
	m_coeffSH[1]->Create(pDevice, maxSumElements, sizeof(float[3]),
		ResourceFlag::ALLOW_UNORDERED_ACCESS, MemoryType::DEFAULT,
		1, nullptr, 1, nullptr, MemoryFlag::NONE, L"SHCoefficients1");
	m_weightSH[0] = StructuredBuffer::MakeUnique(m_api);
	m_weightSH[0]->Create(pDevice, numGroups, sizeof(float),
		ResourceFlag::ALLOW_UNORDERED_ACCESS, MemoryType::DEFAULT,
		1, nullptr, 1, nullptr, MemoryFlag::NONE, L"SHWeights0");
	m_weightSH[1] = StructuredBuffer::MakeUnique(m_api);
	m_weightSH[1]->Create(pDevice, numSumGroups, sizeof(float),
		ResourceFlag::ALLOW_UNORDERED_ACCESS, MemoryType::DEFAULT,
		1, nullptr, 1, nullptr, MemoryFlag::NONE, L"SHWeights1");

	XUSG_N_RETURN(createPipelineLayouts(), false);
	XUSG_N_RETURN(createPipelines(), false);
	XUSG_N_RETURN(createDescriptorTables(), false);

	return true;
}

void SphericalHarmonics_Impl::Transform(CommandList* pCommandList, Resource* pRadiance,
	const DescriptorTable& srvTable, uint8_t order)
{
	// Set Descriptor pools
	const auto descriptorPool = m_descriptorTableCache->GetDescriptorPool(CBV_SRV_UAV_POOL, m_descriptorPoolIndex);
	pCommandList->SetDescriptorPools(1, &descriptorPool);

	shCubeMap(pCommandList, pRadiance, srvTable, order);
	shSum(pCommandList, order);
	shNormalize(pCommandList, order);
}

StructuredBuffer::sptr SphericalHarmonics_Impl::GetSHCoefficients() const
{
	return m_coeffSH[m_shBufferParity];
}

const DescriptorTable& SphericalHarmonics_Impl::GetSHCoeffSRVTable() const
{
	return m_srvTables[UAV_SRV_SH + m_shBufferParity];
}

bool SphericalHarmonics_Impl::createPipelineLayouts()
{
	const auto pSampler = m_descriptorTableCache->GetSampler(ANISOTROPIC_WRAP);

	// SH cube map transform
	{
		auto cb = 0u;
		auto rwSHBuff = 0u;
		auto rwWeight = rwSHBuff + 1;
		auto txCubeMap = 0u;
		auto sampler = 0u;

		// Load shader
		XUSG_N_RETURN(m_shaderPool->CreateShader(Shader::Stage::CS, m_baseCSIndex + SH_CUBE_MAP, CSSHCubeMap, sizeof(CSSHCubeMap)), false);

		// Get compute shader slots
		const auto reflector = m_shaderPool->GetReflector(Shader::Stage::CS, m_baseCSIndex + SH_CUBE_MAP);
		if (reflector && reflector->IsValid())
		{
			// Get constant buffer slots
			cb = reflector->GetResourceBindingPointByName("cb", cb);

			// Get UAV slots
			rwSHBuff = reflector->GetResourceBindingPointByName("g_rwSHBuff", rwSHBuff);
			rwWeight = reflector->GetResourceBindingPointByName("g_rwWeight", rwWeight);

			// Get SRV slot
			txCubeMap = reflector->GetResourceBindingPointByName("g_txCubeMap", txCubeMap);

			// Get sampler slot
			sampler = reflector->GetResourceBindingPointByName("g_sampler", sampler);
		}

		const auto utilPipelineLayout = Util::PipelineLayout::MakeUnique();
		
		assert(rwWeight == rwSHBuff + 1);
		utilPipelineLayout->SetRange(UAV_BUFFERS, DescriptorType::UAV, 2, rwSHBuff, 0,
			DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE | DescriptorFlag::DESCRIPTORS_VOLATILE);

		utilPipelineLayout->SetRange(SRV_BUFFERS, DescriptorType::SRV, 1, txCubeMap);
		utilPipelineLayout->SetConstants(CONSTANTS, XUSG_UINT32_SIZE_OF(uint32_t[2]), cb);
		utilPipelineLayout->SetStaticSamplers(&pSampler, 1, sampler);

		XUSG_X_RETURN(m_pipelineLayouts[SH_CUBE_MAP], utilPipelineLayout->GetPipelineLayout(
			m_pipelineLayoutCache.get(), PipelineLayoutFlag::NONE, L"SHCubeMapLayout"), false);
	}

	// SH sum
	{
		auto cb = 0u;
		auto rwSHBuff = 0u;
		auto rwWeight = rwSHBuff + 1;
		auto roSHBuff = 0u;
		auto roWeight = roSHBuff + 1;

		// Load shader
		XUSG_N_RETURN(m_shaderPool->CreateShader(Shader::Stage::CS, m_baseCSIndex + SH_SUM, CSSHSum, sizeof(CSSHSum)), false);

		// Get compute shader slots
		const auto reflector = m_shaderPool->GetReflector(Shader::Stage::CS, m_baseCSIndex + SH_SUM);
		if (reflector && reflector->IsValid())
		{
			// Get constant buffer slots
			cb = reflector->GetResourceBindingPointByName("cb", cb);

			// Get UAV slots
			rwSHBuff = reflector->GetResourceBindingPointByName("g_rwSHBuff", rwSHBuff);
			rwWeight = reflector->GetResourceBindingPointByName("g_rwWeight", rwWeight);

			// Get SRV slots
			roSHBuff = reflector->GetResourceBindingPointByName("g_roSHBuff", roSHBuff);
			roWeight = reflector->GetResourceBindingPointByName("g_roWeight", roWeight);
		}

		const auto utilPipelineLayout = Util::PipelineLayout::MakeUnique();

		assert(rwWeight == rwSHBuff + 1);
		utilPipelineLayout->SetRange(UAV_BUFFERS, DescriptorType::UAV, 2, rwSHBuff, 0,
			DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE | DescriptorFlag::DESCRIPTORS_VOLATILE);

		assert(roWeight == roSHBuff + 1);
		utilPipelineLayout->SetRange(SRV_BUFFERS, DescriptorType::SRV, 2, roSHBuff);

		utilPipelineLayout->SetConstants(CONSTANTS, XUSG_UINT32_SIZE_OF(uint32_t[2]), cb);

		XUSG_X_RETURN(m_pipelineLayouts[SH_SUM], utilPipelineLayout->GetPipelineLayout(
			m_pipelineLayoutCache.get(), PipelineLayoutFlag::NONE, L"SHSumLayout"), false);
	}

	// SH normalization
	{
		auto rwSHResult = 0u;
		auto roSHBuff = 0u;
		auto roWeight = roSHBuff + 1;

		// Load shader
		XUSG_N_RETURN(m_shaderPool->CreateShader(Shader::Stage::CS, m_baseCSIndex + SH_NORMALIZE, CSSHNormalize, sizeof(CSSHNormalize)), false);

		// Get compute shader slots
		const auto reflector = m_shaderPool->GetReflector(Shader::Stage::CS, m_baseCSIndex + SH_NORMALIZE);
		if (reflector && reflector->IsValid())
		{
			// Get UAV slots
			rwSHResult = reflector->GetResourceBindingPointByName("g_rwSHResult", rwSHResult);

			// Get SRV slots
			roSHBuff = reflector->GetResourceBindingPointByName("g_roSHBuff", roSHBuff);
			roWeight = reflector->GetResourceBindingPointByName("g_roWeight", roWeight);
		}

		const auto utilPipelineLayout = Util::PipelineLayout::MakeUnique();
		utilPipelineLayout->SetRange(UAV_BUFFERS, DescriptorType::UAV, 1, rwSHResult, 0,
			DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE | DescriptorFlag::DESCRIPTORS_VOLATILE);

		assert(roWeight == roSHBuff + 1);
		utilPipelineLayout->SetRange(SRV_BUFFERS, DescriptorType::SRV, 2, roSHBuff);

		XUSG_X_RETURN(m_pipelineLayouts[SH_NORMALIZE], utilPipelineLayout->GetPipelineLayout(
			m_pipelineLayoutCache.get(), PipelineLayoutFlag::NONE, L"SHNormalizeLayout"), false);
	}

	return true;
}

bool SphericalHarmonics_Impl::createPipelines()
{
	// SH cube map transform
	{
		const auto state = Compute::State::MakeUnique();
		state->SetPipelineLayout(m_pipelineLayouts[SH_CUBE_MAP]);
		state->SetShader(m_shaderPool->GetShader(Shader::Stage::CS, m_baseCSIndex + SH_CUBE_MAP));
		XUSG_X_RETURN(m_pipelines[SH_CUBE_MAP], state->GetPipeline(m_computePipelineCache.get(), L"SHCubeMap"), false);
	}

	// SH sum
	{
		const auto state = Compute::State::MakeUnique();
		state->SetPipelineLayout(m_pipelineLayouts[SH_SUM]);
		state->SetShader(m_shaderPool->GetShader(Shader::Stage::CS, m_baseCSIndex + SH_SUM));
		XUSG_X_RETURN(m_pipelines[SH_SUM], state->GetPipeline(m_computePipelineCache.get(), L"SHSum"), false);
	}

	// SH normalization
	{
		const auto state = Compute::State::MakeUnique();
		state->SetPipelineLayout(m_pipelineLayouts[SH_NORMALIZE]);
		state->SetShader(m_shaderPool->GetShader(Shader::Stage::CS, m_baseCSIndex + SH_NORMALIZE));
		XUSG_X_RETURN(m_pipelines[SH_NORMALIZE], state->GetPipeline(m_computePipelineCache.get(), L"SHNormalize"), false);
	}

	return true;
}

bool SphericalHarmonics_Impl::createDescriptorTables()
{
	// Create SH UAV tables
	for (uint8_t i = 0; i < 2; ++i)
	{
		const auto descriptorTable = Util::DescriptorTable::MakeUnique(m_api);
		const Descriptor descriptors[] =
		{
			m_coeffSH[i]->GetUAV(),
			m_weightSH[i]->GetUAV()
		};
		descriptorTable->SetDescriptors(0, static_cast<uint32_t>(size(descriptors)), descriptors, m_descriptorPoolIndex);
		XUSG_X_RETURN(m_uavTables[UAV_SRV_SH + i], descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get()), false);
	}

	// Create SH SRV tables
	for (uint8_t i = 0; i < 2; ++i)
	{
		const auto descriptorTable = Util::DescriptorTable::MakeUnique(m_api);
		const Descriptor descriptors[] =
		{
			m_coeffSH[i]->GetSRV(),
			m_weightSH[i]->GetSRV()
		};
		descriptorTable->SetDescriptors(0, static_cast<uint32_t>(size(descriptors)), descriptors, m_descriptorPoolIndex);
		XUSG_X_RETURN(m_srvTables[UAV_SRV_SH + i], descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get()), false);
	}

	return true;
}

void SphericalHarmonics_Impl::shCubeMap(CommandList* pCommandList,
	Resource* pRadiance, const DescriptorTable& srvTable, uint8_t order)
{
	assert(order <= SH_MAX_ORDER);

	// Set barriers
	ResourceBarrier barriers[3];
	auto numBarriers = m_coeffSH[0]->SetBarrier(barriers, ResourceState::UNORDERED_ACCESS,
		0, XUSG_BARRIER_ALL_SUBRESOURCES, BarrierFlag::RESET_SRC_STATE);
	numBarriers = m_weightSH[0]->SetBarrier(barriers, ResourceState::UNORDERED_ACCESS,
		numBarriers, XUSG_BARRIER_ALL_SUBRESOURCES, BarrierFlag::RESET_SRC_STATE);
	numBarriers = pRadiance->SetBarrier(barriers, ResourceState::SHADER_RESOURCE, numBarriers);
	pCommandList->Barrier(numBarriers, barriers);

	// Set pipeline layout and descriptor tables
	pCommandList->SetComputePipelineLayout(m_pipelineLayouts[SH_CUBE_MAP]);
	pCommandList->SetComputeDescriptorTable(UAV_BUFFERS, m_uavTables[UAV_SRV_SH]);
	pCommandList->SetComputeDescriptorTable(SRV_BUFFERS, srvTable);
	pCommandList->SetCompute32BitConstant(CONSTANTS, order);
	pCommandList->SetCompute32BitConstant(CONSTANTS, SH_TEX_SIZE, XUSG_UINT32_SIZE_OF(order));

	// Set pipeline
	pCommandList->SetPipelineState(m_pipelines[SH_CUBE_MAP]);

	// Dispatch
	pCommandList->Dispatch(XUSG_DIV_UP(m_numSHTexels, SH_GROUP_SIZE), 1, 1);
}

void SphericalHarmonics_Impl::shSum(CommandList* pCommandList, uint8_t order)
{
	assert(order <= SH_MAX_ORDER);
	ResourceBarrier barriers[4];
	m_shBufferParity = 0;

	// Set pipeline layout and pipeline
	pCommandList->SetComputePipelineLayout(m_pipelineLayouts[SH_SUM]);
	pCommandList->SetCompute32BitConstant(CONSTANTS, order);
	pCommandList->SetPipelineState(m_pipelines[SH_SUM]);

	auto barrierFlag = BarrierFlag::RESET_SRC_STATE;
	for (auto n = XUSG_DIV_UP(m_numSHTexels, SH_GROUP_SIZE); n > 1; n = XUSG_DIV_UP(n, SH_GROUP_SIZE))
	{
		// Set barriers
		const auto& src = m_shBufferParity;
		const uint8_t dst = !m_shBufferParity;
		auto numBarriers = m_coeffSH[dst]->SetBarrier(barriers, ResourceState::UNORDERED_ACCESS,
			0, XUSG_BARRIER_ALL_SUBRESOURCES, barrierFlag);
		numBarriers = m_weightSH[dst]->SetBarrier(barriers, ResourceState::UNORDERED_ACCESS,
			numBarriers, XUSG_BARRIER_ALL_SUBRESOURCES, barrierFlag);
		numBarriers = m_coeffSH[src]->SetBarrier(barriers, ResourceState::NON_PIXEL_SHADER_RESOURCE, numBarriers);
		numBarriers = m_weightSH[src]->SetBarrier(barriers, ResourceState::NON_PIXEL_SHADER_RESOURCE, numBarriers);
		pCommandList->Barrier(numBarriers, barriers);

		// Set descriptor tables
		pCommandList->SetComputeDescriptorTable(UAV_BUFFERS, m_uavTables[UAV_SRV_SH + dst]);
		pCommandList->SetComputeDescriptorTable(SRV_BUFFERS, m_srvTables[UAV_SRV_SH + src]);
		pCommandList->SetCompute32BitConstant(CONSTANTS, n, XUSG_UINT32_SIZE_OF(order));

		// Dispatch
		pCommandList->Dispatch(XUSG_DIV_UP(n, SH_GROUP_SIZE), order * order, 1);
		m_shBufferParity = !m_shBufferParity;
		barrierFlag = BarrierFlag::NONE;
	}
}

void SphericalHarmonics_Impl::shNormalize(CommandList* pCommandList, uint8_t order)
{
	assert(order <= SH_MAX_ORDER);

	// Set barriers
	ResourceBarrier barriers[3];
	const auto& src = m_shBufferParity;
	const uint8_t dst = !m_shBufferParity;
	auto numBarriers = m_coeffSH[dst]->SetBarrier(barriers, ResourceState::UNORDERED_ACCESS);
	numBarriers = m_coeffSH[src]->SetBarrier(barriers, ResourceState::NON_PIXEL_SHADER_RESOURCE, numBarriers);
	numBarriers = m_weightSH[src]->SetBarrier(barriers, ResourceState::NON_PIXEL_SHADER_RESOURCE, numBarriers);
	pCommandList->Barrier(numBarriers, barriers);

	// Set pipeline layout and descriptor tables
	pCommandList->SetComputePipelineLayout(m_pipelineLayouts[SH_NORMALIZE]);
	pCommandList->SetComputeDescriptorTable(UAV_BUFFERS, m_uavTables[UAV_SRV_SH + dst]);
	pCommandList->SetComputeDescriptorTable(SRV_BUFFERS, m_srvTables[UAV_SRV_SH + src]);

	// Set pipeline
	pCommandList->SetPipelineState(m_pipelines[SH_NORMALIZE]);

	// Dispatch
	const auto numElements = order * order;
	pCommandList->Dispatch(XUSG_DIV_UP(numElements, SH_GROUP_SIZE), 1, 1);
	m_shBufferParity = !m_shBufferParity;
}
