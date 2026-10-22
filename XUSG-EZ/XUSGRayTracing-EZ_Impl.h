//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing-EZ.h"
#include "XUSG-EZ_Impl.h"
#include "XUSGUltimate-EZ_Impl.h"
#include "RayTracing/XUSGRayTracingCommand_DX12.h"
#ifndef XUSG_NO_VULKAN
#include "RayTracing/XUSGRayTracingCommand_VK.h"
#endif

namespace XUSG
{
	namespace RayTracing
	{
		namespace EZ
		{
			template<typename CoreList, typename UltimateList, typename RTList>
			class CommandList_Impl :
				virtual public CommandList,
				virtual public RTList,
				virtual public Ultimate::EZ::CommandList_Impl<CoreList, UltimateList>
			{
			public:
				CommandList_Impl();
				CommandList_Impl(RayTracing::CommandList* pCommandList, uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize);
				virtual ~CommandList_Impl();

				bool Create(RayTracing::CommandList* pCommandList, uint32_t samplerHeapSize, uint32_t cbvSrvUavHeapSize);
				bool Create(RayTracing::Device* pDevice, void* pNativeHandle, uint32_t samplerHeapSize,
					uint32_t cbvSrvUavHeapSize);
				bool CreatePipelineLayouts(
					const uint32_t samplerStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSamplers[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxCbvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxSrvsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t* pMaxUavsEachSpace[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t cbvStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxCbvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t srvStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxSrvSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t uavStartSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t maxUavSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t max32BitConstants[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSlots[Shader::Stage::NUM_STAGE] = nullptr,
					const uint32_t constantSpaces[Shader::Stage::NUM_STAGE] = nullptr,
					uint32_t maxTLASSrvs = 0, uint32_t spaceTLAS = 0,
					uint32_t slotExt = 0, uint32_t spaceExt = 0x7FFF0ADE);
				bool Reset(uint8_t allocatorIndex, const Pipeline& initialState);
				bool PrebuildBLAS(BottomLevelAS* pBLAS, uint32_t numGeometries, const GeometryBuffer& geometries,
					BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);
				bool PrebuildTLAS(TopLevelAS* pTLAS, uint32_t numInstances,
					BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);
				bool PrebuildOmmArray(OpacityMicromapArray* pOmmArray, uint32_t numOpacityMicromaps,
					const GeometryBuffer& ommArrayDescs, BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);
				bool AllocateAccelerationStructure(AccelerationStructure* pAccelerationStructure, size_t byteWidth = 0);

				void SetTriangleGeometry(BottomLevelAS::GeometryDesc& geometry, Format vertexFormat,
					const XUSG::EZ::VertexBufferView& vbv, const XUSG::EZ::IndexBufferView* pIbv = nullptr,
					GeometryFlag flags = GeometryFlag::FULL_OPAQUE, const ResourceView* pTransform = nullptr);
				void SetAABBGeometry(BottomLevelAS::GeometryDesc& geometry, const XUSG::EZ::VertexBufferView& vbv,
					GeometryFlag flags = GeometryFlag::NONE);
				void SetOMMGeometry(BottomLevelAS::GeometryDesc& geometry, const BottomLevelAS::TriangleGeometry* pTriangles,
					const BottomLevelAS::OMMLinkage* pOmmLinkage, Buffer* pOpacityMicromapIndexBuffer,
					GeometryFlag flags = GeometryFlag::NONE);
				void SetOmmArray(GeometryBuffer& ommArrayDescs, uint32_t numOpacityMicromaps,
					const OpacityMicromapArray::Desc* pOmmArrayDescs);
				void SetBLASDestination(BottomLevelAS* pBLAS, const Buffer::sptr destBuffer,
					uintptr_t byteOffset, uint32_t uavIndex);
				void SetTLASDestination(TopLevelAS* pTLAS, const Buffer::sptr destBuffer,
					uintptr_t byteOffset, uint32_t uavIndex, uint32_t srvIndex);
				void SetOmmArrayDestination(OpacityMicromapArray* pOmmArray, const Buffer::sptr destBuffer, uintptr_t byteOffset) const;
				void BuildBLAS(BottomLevelAS* pBLAS, BottomLevelAS* pSource = nullptr,
					uint8_t numPostbuildInfoDescs = 0, const PostbuildInfoType* pPostbuildInfoTypes = nullptr);
				void BuildTLAS(TopLevelAS* pTLAS, Resource* pInstanceDescs, TopLevelAS* pSource = nullptr,
					uint8_t numPostbuildInfoDescs = 0, const PostbuildInfoType* pPostbuildInfoTypes = nullptr);
				void BuildOmmArray(OpacityMicromapArray* pOmmArray, OpacityMicromapArray* pSource = nullptr,
					uint8_t numPostbuildInfoDescs = 0, const PostbuildInfoType* pPostbuildInfoTypes = nullptr);
				void CopyRaytracingAccelerationStructure(AccelerationStructure* pDst,
					AccelerationStructure* pSrc, CopyMode mode);
				void SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const;
				void RTSetShaderLibrary(uint32_t index, const Blob& shaderLib,
					uint32_t numShaders = 0, const wchar_t** pShaderNames = nullptr);
				void RTSetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize = sizeof(float[2]));
				void RTSetHitGroup(uint32_t index, const wchar_t* hitGroupName, const wchar_t* closestHitShaderName,
					const wchar_t* anyHitShaderName = nullptr, const wchar_t* intersectionShaderName = nullptr,
					HitGroupType type = HitGroupType::TRIANGLES);
				void RTSetMaxRecursionDepth(uint32_t depth);
				void RTSetPipelineState(const Pipeline& pipelineState, RayTracing::State* pState = nullptr);
				void DispatchRays(uint32_t width, uint32_t height, uint32_t depth, const wchar_t* rayGenShaderName,
					const wchar_t* const* pMissShaderNames, uint32_t numMissShaders,
					const wchar_t* const* pCallableShaderNames = nullptr, uint32_t numCallableShaders = 0);
				void DispatchRaysIndirect(CommandLayout* pCommandlayout,
					uint32_t maxCommandCount,
					const wchar_t* rayGenShaderName,
					const wchar_t* const* pMissShaderNames,
					uint32_t numMissShaders,
					const wchar_t* const* pCallableShaderNames,
					uint32_t numCallableShaders,
					Resource* pArgumentBuffer,
					uint64_t argumentBufferOffset = 0,
					Resource* pCountBuffer = nullptr,
					uint64_t countBufferOffset = 0);

				size_t GetDispatchRaysArgReservedOffset() const { return sizeof(DX12DispatchRaysDescHeader); }

				Device* GetRTDevice() const { return RTList::GetRTDevice(); }
				RayTracing::CommandList* AsRTCommandList() const
				{
					return const_cast<RayTracing::CommandList*>(dynamic_cast<const RayTracing::CommandList*>(this));
				}

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

				virtual bool bindRayTracing(RayTracing::CommandList* pCommandList) = 0;
				virtual bool supportsMeshShader() const = 0;

				using UltimateEZ = Ultimate::EZ::CommandList_Impl<CoreList, UltimateList>;
				using UltimateEZ::createGraphicsPipelineLayouts;
				using UltimateEZ::createMeshShaderPipelineLayouts;
				using UltimateEZ::preexecuteIndirect;
				using UltimateEZ::clearUAVs;
				using UltimateEZ::COMPUTE;
				using UltimateEZ::m_cbvSrvUavTables;
				using UltimateEZ::m_computeSpaceToParamIndexMap;
				using UltimateEZ::m_computeConstantParamIndex;
				using UltimateEZ::m_pipelineLayouts;
				using UltimateEZ::m_pipelineLayoutLib;
				using UltimateEZ::m_pipeline;
				using UltimateEZ::m_isComputeDirty;
				using UltimateEZ::m_barriers;
				using UltimateEZ::m_descriptorTableLib;
				using UltimateEZ::m_samplerTables;
				using UltimateEZ::m_spaceStarts;
				using UltimateEZ::recordSpaceStarts;
				using UltimateEZ::spaceStart;
				using UltimateEZ::CbvSrvUavTypes;
				using CoreList::m_pDevice;
				using RTList::m_pDeviceRT;

				bool createComputePipelineLayouts(uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace,
					const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
					uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces,
					uint32_t max32BitConstants, uint32_t constantSlot, uint32_t constantSpace,
					uint32_t maxTLASSrvs, uint32_t spaceTLAS, uint32_t slotExt, uint32_t spaceExt);

				void predispatchRays(CShaderTablePtr& pRayGen, CShaderTablePtr& pHitGroup, CShaderTablePtr& pMiss, CShaderTablePtr& pCallable,
					const wchar_t* rayGenShaderName, const wchar_t* const* pMissShaderNames, uint32_t numMissShaders,
					const wchar_t* const* pCallableShaderNames, uint32_t numCallableShaders);

				Resource* needScratch(size_t size);

				const ShaderTable* getShaderTable(const std::string& key,
					std::unordered_map<std::string, ShaderTable::uptr>& shaderTables,
					uint32_t numShaderIDs);

				RayTracing::PipelineLib::uptr m_rayTracingPipelineLib;

				size_t m_scratchSize;
				std::vector<Buffer::uptr> m_scratches;

				RayTracing::State::uptr m_rayTracingState;
				bool m_isRTStateDirty;

				std::vector<uint32_t> m_tlasBindingToParamIndexMap;

				std::vector<Buffer::uptr> m_uploaders;

				std::unordered_map<std::string, ShaderTable::uptr> m_rayGenTables;
				std::unordered_map<std::string, ShaderTable::uptr> m_hitGroupTables;
				std::unordered_map<std::string, ShaderTable::uptr> m_missTables;
				std::unordered_map<std::string, ShaderTable::uptr> m_callableTables;
				const ShaderTable* m_pHitGroup;

				std::unordered_set<std::string> m_argumentBufferVAs;
			};

			extern template class CommandList_Impl<XUSG::CommandList_DX12, Ultimate::CommandList_DX12, RayTracing::CommandList_DX12>;
#ifndef XUSG_NO_VULKAN
			extern template class CommandList_Impl<XUSG::CommandList_VK, Ultimate::CommandList_VK, RayTracing::CommandList_VK>;
#endif

			class CommandList_DX12 :
				public virtual CommandList_Impl<XUSG::CommandList_DX12, Ultimate::CommandList_DX12, RayTracing::CommandList_DX12>,
				public virtual Ultimate::EZ::CommandList_DX12
			{
			public:
				using Impl = CommandList_Impl<XUSG::CommandList_DX12, Ultimate::CommandList_DX12, RayTracing::CommandList_DX12>;
				using Impl::Impl;

			protected:
				bool bindRayTracing(RayTracing::CommandList* pCommandList) override;
				bool supportsMeshShader() const override;
			};

#ifndef XUSG_NO_VULKAN
			class CommandList_VK :
				public virtual CommandList_Impl<XUSG::CommandList_VK, Ultimate::CommandList_VK, RayTracing::CommandList_VK>,
				public virtual Ultimate::EZ::CommandList_VK
			{
			public:
				using Impl = CommandList_Impl<XUSG::CommandList_VK, Ultimate::CommandList_VK, RayTracing::CommandList_VK>;
				using Impl::Impl;

			protected:
				bool bindRayTracing(RayTracing::CommandList* pCommandList) override;
				bool supportsMeshShader() const override;
			};
#endif
		}
	}
}
