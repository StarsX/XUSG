//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing-EZ.h"

namespace XUSG
{
	namespace RayTracing
	{
		namespace EZ
		{
			class CommandList_DXR :
				virtual public CommandList,
				virtual public RayTracing::CommandList_DX12,
				virtual public Ultimate::EZ::CommandList_DX12
			{
			public:
				CommandList_DXR();
				CommandList_DXR(RayTracing::CommandList* pCommandList,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0);
				virtual ~CommandList_DXR();

				bool Create(RayTracing::CommandList* pCommandList,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0);
				bool Create(const RayTracing::Device* pDevice, void* pHandle,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0,
					const wchar_t* name = nullptr);
				bool Reset(const CommandAllocator* pAllocator, const Pipeline& initialState);
				bool PreBuildBLAS(BottomLevelAS* pBLAS, uint32_t numGeometries, const GeometryBuffer& geometries,
					BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);
				bool PreBuildTLAS(TopLevelAS* pTLAS, uint32_t numInstances,
					BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);

				void SetTriangleGeometries(GeometryBuffer& geometries, uint32_t numGeometries, Format vertexFormat,
					XUSG::EZ::VertexBufferView* pVBs, XUSG::EZ::IndexBufferView* pIBs = nullptr,
					const GeometryFlag* pGeometryFlags = nullptr, const ResourceView* pTransforms = nullptr);
				void SetAABBGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
					XUSG::EZ::VertexBufferView* pVBs, const GeometryFlag* pGeometryFlags = nullptr);
				void BuildBLAS(BottomLevelAS* pBLAS, const BottomLevelAS* pSource = nullptr);
				void BuildTLAS(TopLevelAS* pTLAS, const Resource* pInstanceDescs, const TopLevelAS* pSource = nullptr);
				void SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const;
				void RTSetShaderLibrary(uint32_t index, const Blob& shaderLib,
					uint32_t numShaders = 0, const wchar_t** pShaderNames = nullptr);
				void RTSetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize = sizeof(float[2]));
				void RTSetHitGroup(uint32_t index, const wchar_t* hitGroupName, const wchar_t* closestHitShaderName,
					const wchar_t* anyHitShaderName = nullptr, const wchar_t* intersectionShaderName = nullptr,
					HitGroupType type = HitGroupType::TRIANGLES);
				void RTSetMaxRecursionDepth(uint32_t depth);
				void DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
					const wchar_t* rayGenShaderName, const wchar_t* missShaderName);
				void DispatchRaysIndirect(const CommandLayout* pCommandlayout,
					uint32_t maxCommandCount,
					const wchar_t* rayGenShaderName,
					const wchar_t* missShaderName,
					Resource* pArgumentBuffer,
					uint64_t argumentBufferOffset = 0,
					Resource* pCountBuffer = nullptr,
					uint64_t countBufferOffset = 0);

				size_t GetDispatchRaysArgReservedOffset() const { return sizeof(DX12DispatchRaysDescHeader); }

				const Device* GetRTDevice() const { return RayTracing::CommandList_DX12::GetRTDevice(); }
				RayTracing::CommandList* AsRTCommandList() { return dynamic_cast<RayTracing::CommandList*>(this); }

			protected:
				//using ShaderTablePtr = ShaderTable*;
				using CShaderTablePtr = const ShaderTable*;

				struct DX12DispatchRaysDescHeader
				{
					D3D12_GPU_VIRTUAL_ADDRESS_RANGE RayGenerationShaderRecord;
					D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE MissShaderTable;
					D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE HitGroupTable;
					D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CallableShaderTable;
				};

				struct ArgumentBufferVA
				{
					Resource* pBuffer;
					uint64_t Offset;
				};

				bool createComputePipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
					const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
					uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
					uint32_t maxTLASSrvs, uint32_t spaceTLAS);

				void predispatchRays(CShaderTablePtr& pRayGen, CShaderTablePtr& pHitGroup, CShaderTablePtr& pMiss,
					const wchar_t* rayGenShaderName, const wchar_t* missShaderName);

				Resource* needScratch(uint32_t size);

				const ShaderTable* getShaderTable(const std::string& key,
					std::unordered_map<std::string, ShaderTable::uptr>& shaderTables,
					uint32_t numShaderIDs);

				RayTracing::PipelineLib::uptr m_RayTracingPipelineLib;

				uint32_t m_scratchSize;
				std::vector<Resource::uptr> m_scratches;

				RayTracing::State::uptr m_rayTracingState;
				bool m_isRTStateDirty;

				uint32_t m_asUavCount;
				std::vector<uint32_t> m_tlasBindingToParamIndexMap;

				std::vector<Buffer::uptr> m_uploaders;

				std::unordered_map<std::string, ShaderTable::uptr> m_rayGenTables;
				std::unordered_map<std::string, ShaderTable::uptr> m_hitGroupTables;
				std::unordered_map<std::string, ShaderTable::uptr> m_missTables;

				std::unordered_set<std::string> m_argumentBufferVAs;
			};
		}
	}
}
