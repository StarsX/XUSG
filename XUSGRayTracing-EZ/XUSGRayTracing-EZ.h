//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGUltimate-EZ.h"
#include "RayTracing/XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		namespace EZ
		{
			XUSG_INTERFACE XUSG::EZ::ResourceView GetSRV(AccelerationStructure* pAS);

			//--------------------------------------------------------------------------------------
			// RayTracing command list
			//--------------------------------------------------------------------------------------
			class XUSG_INTERFACE CommandList :
				public virtual Ultimate::EZ::CommandList
			{
			public:
				//CommandList();
				virtual ~CommandList() {}

				using uptr = std::unique_ptr<CommandList>;
				using sptr = std::shared_ptr<CommandList>;

				// By default maxCbvsEachSpace[stage] = 14 for graphics or 12 for ray tracing and compute
				// maxSamplers[stage] = 16, maxSrvsEachSpace[stage] = 32, and maxUavsEachSpace[stage] = 16
				virtual bool Create(RayTracing::CommandList* pCommandList,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0) = 0;
				virtual bool Create(const RayTracing::Device* pDevice, void* pHandle,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0,
					const wchar_t* name = nullptr) = 0;
				virtual bool Reset(const CommandAllocator* pAllocator, const Pipeline& initialState) = 0;
				virtual bool PreBuildBLAS(BottomLevelAS* pBLAS, uint32_t numGeometries, const GeometryBuffer& geometries,
					BuildFlag flags = BuildFlag::PREFER_FAST_TRACE) = 0;
				virtual bool PreBuildTLAS(TopLevelAS* pTLAS, uint32_t numInstances,
					BuildFlag flags = BuildFlag::PREFER_FAST_TRACE) = 0;

				virtual void SetTriangleGeometries(GeometryBuffer& geometries, uint32_t numGeometries, Format vertexFormat,
					XUSG::EZ::VertexBufferView* pVBs, XUSG::EZ::IndexBufferView* pIBs = nullptr,
					const GeometryFlag* pGeometryFlags = nullptr, const ResourceView* pTransforms = nullptr) = 0;
				virtual void SetAABBGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
					XUSG::EZ::VertexBufferView* pVBs, const GeometryFlag* pGeometryFlags = nullptr) = 0;
				virtual void BuildBLAS(BottomLevelAS* pBLAS, const BottomLevelAS* pSource = nullptr) = 0;
				virtual void BuildTLAS(TopLevelAS* pTLAS, const Resource* pInstanceDescs, const TopLevelAS* pSource = nullptr) = 0;
				virtual void SetTopLevelAccelerationStructure(uint32_t binding, const TopLevelAS* pTopLevelAS) const = 0;
				virtual void RTSetShaderLibrary(uint32_t index, const Blob& shaderLib,
					uint32_t numShaders = 0, const void** pShaders = nullptr) = 0;
				virtual void RTSetHitGroup(uint32_t index, const void* pHitGroup, const void* pClosestHitShader,
					const void* pAnyHitShader = nullptr, const void* pIntersectionShader = nullptr,
					HitGroupType type = HitGroupType::TRIANGLES) = 0;
				virtual void RTSetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize = sizeof(float[2])) = 0;
				virtual void RTSetMaxRecursionDepth(uint32_t depth) = 0;
				virtual void DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
					const void* pRayGenShader, const void* pMissShader) = 0;
				virtual void DispatchRaysIndirect(const CommandLayout* pCommandlayout,
					uint32_t maxCommandCount,
					const void* pRayGenShader,
					const void* pMissShader,
					Resource* pArgumentBuffer,
					uint64_t argumentBufferOffset = 0,
					Resource* pCountBuffer = nullptr,
					uint64_t countBufferOffset = 0) = 0;

				virtual size_t GetDispatchRaysArgReservedOffset() const = 0;

				virtual const Device* GetRTDevice() const = 0;
				virtual RayTracing::CommandList* AsRTCommandList() = 0;

				static uptr MakeUnique(API api = API::DIRECTX_12);
				static sptr MakeShared(API api = API::DIRECTX_12);
				static uptr MakeUnique(RayTracing::CommandList* pCommandList,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0,
					API api = API::DIRECTX_12);
				static sptr MakeShared(RayTracing::CommandList* pCommandList,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0,
					API api = API::DIRECTX_12);
			};
		}
	}
}
