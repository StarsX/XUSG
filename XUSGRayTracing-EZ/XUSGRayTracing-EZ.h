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
			XUSG_INTERFACE XUSG::EZ::ResourceView GetSRV(TopLevelAS* pTLAS);

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

				virtual bool Create(RayTracing::CommandList* pCommandList,
					uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize) = 0;
				virtual bool Create(const RayTracing::Device* pDevice, void* pHandle, uint32_t samplerHeapSize,
					uint32_t cbvSrvUavHeapSize, const wchar_t* name = nullptr) = 0;
				// Must be called after all acceleration structures allocated
				// By default maxCbvsEachSpace[stage] = 14 for graphics or 12 for ray tracing and compute
				// maxSamplers[stage] = 16, maxSrvsEachSpace[stage] = 32, and maxUavsEachSpace[stage] = 16
				virtual bool CreatePipelineLayouts(
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSlots[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0,
					uint32_t slotExt = 0, uint32_t spaceExt = 0x7FFF0ADE) = 0;
				virtual bool Reset(const CommandAllocator* pAllocator, const Pipeline& initialState) = 0;
				virtual bool PrebuildBLAS(BottomLevelAS* pBLAS, uint32_t numGeometries, const GeometryBuffer& geometries,
					BuildFlag flags = BuildFlag::PREFER_FAST_TRACE) = 0;
				virtual bool PrebuildTLAS(TopLevelAS* pTLAS, uint32_t numInstances,
					BuildFlag flags = BuildFlag::PREFER_FAST_TRACE) = 0;

				// Auto allocate a buffer with byteWidth = GetResultDataMaxByteSize() when setting byteWidth = 0
				virtual bool AllocateAccelerationStructure(AccelerationStructure* pAccelerationStructure, size_t byteWidth = 0) = 0;

				virtual void SetTriangleGeometries(GeometryBuffer& geometries, uint32_t numGeometries, Format vertexFormat,
					XUSG::EZ::VertexBufferView* pVBs, XUSG::EZ::IndexBufferView* pIBs = nullptr,
					const GeometryFlag* pGeometryFlags = nullptr, const ResourceView* pTransforms = nullptr) = 0;
				virtual void SetAABBGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
					XUSG::EZ::VertexBufferView* pVBs, const GeometryFlag* pGeometryFlags = nullptr) = 0;
				virtual void SetBLASDestination(BottomLevelAS* pBLAS, const Buffer::sptr destBuffer,
					uintptr_t byteOffset, uint32_t uavIndex) = 0;
				virtual void SetTLASDestination(TopLevelAS* pTLAS, const Buffer::sptr destBuffer,
					uintptr_t byteOffset, uint32_t uavIndex, uint32_t srvIndex) = 0;
				virtual void BuildBLAS(BottomLevelAS* pBLAS, const BottomLevelAS* pSource = nullptr,
					uint8_t numPostbuildInfoDescs = 0, const PostbuildInfoType* pPostbuildInfoTypes = nullptr) = 0;
				virtual void BuildTLAS(TopLevelAS* pTLAS, const Resource* pInstanceDescs, const TopLevelAS* pSource = nullptr,
					uint8_t numPostbuildInfoDescs = 0, const PostbuildInfoType* pPostbuildInfoTypes = nullptr) = 0;
				virtual void CopyRaytracingAccelerationStructure(const AccelerationStructure* pDst,
					const AccelerationStructure* pSrc, CopyMode mode) = 0;
				virtual void SetTopLevelAccelerationStructure(uint32_t binding, const TopLevelAS* pTopLevelAS) const = 0;
				virtual void RTSetShaderLibrary(uint32_t index, const Blob& shaderLib,
					uint32_t numShaders = 0, const wchar_t** pShaderNames = nullptr) = 0;
				virtual void RTSetHitGroup(uint32_t index, const wchar_t* hitGroupName, const wchar_t* closestHitShaderName,
					const wchar_t* anyHitShaderName = nullptr, const wchar_t* intersectionShaderName = nullptr,
					HitGroupType type = HitGroupType::TRIANGLES) = 0;
				virtual void RTSetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize = sizeof(float[2])) = 0;
				virtual void RTSetMaxRecursionDepth(uint32_t depth) = 0;
				virtual void RTSetPipelineState(const Pipeline& pipelineState, RayTracing::State* pState = nullptr) = 0;
				virtual void DispatchRays(uint32_t width, uint32_t height, uint32_t depth, const wchar_t* rayGenShaderName,
					const wchar_t* const* pMissShaderNames, uint32_t numMissShaders) = 0;
				virtual void DispatchRaysIndirect(const CommandLayout* pCommandlayout,
					uint32_t maxCommandCount,
					const wchar_t* rayGenShaderName,
					const wchar_t* const* pMissShaderNames,
					uint32_t numMissShaders,
					Resource* pArgumentBuffer,
					uint64_t argumentBufferOffset = 0,
					Resource* pCountBuffer = nullptr,
					uint64_t countBufferOffset = 0) = 0;

				virtual size_t GetDispatchRaysArgReservedOffset() const = 0;

				virtual const Device* GetRTDevice() const = 0;
				virtual RayTracing::CommandList* AsRTCommandList() = 0;

				static uptr MakeUnique(API api = API::DIRECTX_12);
				static sptr MakeShared(API api = API::DIRECTX_12);
				static uptr MakeUnique(RayTracing::CommandList* pCommandList, uint32_t samplerHeapSize,
					uint32_t cbvSrvUavHeapSize, API api = API::DIRECTX_12);
				static sptr MakeShared(RayTracing::CommandList* pCommandList, uint32_t samplerHeapSize,
					uint32_t cbvSrvUavHeapSize, API api = API::DIRECTX_12);
			};
		}
	}
}
