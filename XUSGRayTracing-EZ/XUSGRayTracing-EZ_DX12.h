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
				virtual public XUSG::EZ::CommandList_DX12
			{
			public:
				CommandList_DXR();
				CommandList_DXR(RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0);
				virtual ~CommandList_DXR();

				bool Create(RayTracing::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0);
				bool Create(const RayTracing::Device* pDevice, void* pHandle, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0, const wchar_t* name = nullptr);
				bool Close();
				bool CloseForPresent(RenderTarget* pBackBuffer);
				bool PreBuildBLAS(BottomLevelAS* pBLAS, uint32_t numGeometries, const GeometryBuffer& geometries,
					BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);
				bool PreBuildTLAS(TopLevelAS* pTLAS, uint32_t numInstances,
					BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);

				void SetTriangleGeometries(GeometryBuffer& geometries, uint32_t numGeometries, Format vertexFormat,
					XUSG::EZ::VertexBufferView* pVBs, XUSG::EZ::IndexBufferView* pIBs = nullptr,
					const GeometryFlag* pGeometryFlags = nullptr, const ResourceView* pTransforms = nullptr);
				void SetAABBGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
					XUSG::EZ::VertexBufferView* pVBs, const GeometryFlag* pGeometryFlags = nullptr);
				void BuildBLAS(BottomLevelAS* pBLAS, bool update = false);
				void BuildTLAS(TopLevelAS* pTLAS, const Resource* pInstanceDescs, bool update = false);
				void SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const;
				void RTSetShaderLibrary(Blob shaderLib);
				void RTSetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize = sizeof(float[2]));
				void RTSetHitGroup(uint32_t index, const void* hitGroup, const void* closestHitShader,
					const void* anyHitShader = nullptr, const void* intersectionShader = nullptr,
					HitGroupType type = HitGroupType::TRIANGLES);
				void RTSetMaxRecursionDepth(uint32_t depth);
				void DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
					const void* rayGenShader, const void* missShader);

				const Device* GetRTDevice() const { return RayTracing::CommandList_DX12::GetRTDevice(); }
				RayTracing::CommandList* AsRTCommandList() { return dynamic_cast<RayTracing::CommandList*>(this); }

			protected:
				bool createPipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
					const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
					uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
					uint32_t maxTLASSrvs, uint32_t spaceTLAS);
				bool createComputePipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
					const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
					uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
					uint32_t maxTLASSrvs, uint32_t spaceTLAS);

				Resource* needScratch(uint32_t size);

				const void* getHitGroupFromState(uint32_t index, const RayTracing::State* pState);

				uint32_t getNumHitGroupsFromState(const RayTracing::State* pState);

				const ShaderTable* getShaderTable(const std::string& key,
					std::unordered_map<std::string, ShaderTable::uptr>& shaderTables,
					uint32_t numShaderIDs);

				RayTracing::PipelineCache::uptr m_RayTracingPipelineCache;

				uint32_t m_scratchSize;
				std::vector<Resource::uptr> m_scratches;

				bool m_isRTStateDirty;
				RayTracing::State::uptr m_rayTracingState;

				uint32_t m_asUavCount;
				std::vector<uint32_t> m_tlasBindingToParamIndexMap;

				std::unordered_map<std::string, ShaderTable::uptr> m_rayGenTables;
				std::unordered_map<std::string, ShaderTable::uptr> m_hitGroupTables;
				std::unordered_map<std::string, ShaderTable::uptr> m_missTables;
			};
		}
	}
}
