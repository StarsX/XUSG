//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		class CommandList_DX12 :
			public virtual CommandList,
			public virtual Ultimate::CommandList_DX12
		{
		public:
			CommandList_DX12();
			CommandList_DX12(XUSG::CommandList* pCommandList, const Device* pDevice);
			virtual ~CommandList_DX12();

			bool CreateInterface();

			void BuildRaytracingAccelerationStructure(const BuildDesc* pDesc,
				uint32_t numPostbuildInfoDescs, const PostbuildInfo* pPostbuildInfoDescs,
				const DescriptorHeap* pDescriptorHeap = nullptr);
			void EmitRaytracingAccelerationStructurePostbuildInfo(const PostbuildInfo* pDesc,
				uint32_t numAccelerationStructures, const uint64_t* pAccelerationStructureData) const;
			void CopyRaytracingAccelerationStructure(const AccelerationStructure* pDst,
				const AccelerationStructure* pSrc, CopyMode mode,
				const DescriptorHeap* pDescriptorHeap = nullptr);

			void SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps);
			void SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const;
			void SetTopLevelAccelerationStructure(uint32_t index, uint64_t topLevelASPtr) const;
			void SetRayTracingPipeline(const Pipeline& pipeline) const;
			void DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
				const ShaderTable* pRayGen, const ShaderTable* pHitGroup, const ShaderTable* pMiss,
				const ShaderTable* pCallable = nullptr) const;
			void DispatchRays(const Pipeline& pipeline, uint32_t width, uint32_t height, uint32_t depth,
				const ShaderTable* pRayGen, const ShaderTable* pHitGroup, const ShaderTable* pMiss,
				const ShaderTable* pCallable = nullptr) const;

			const Device* GetRTDevice() const;

		protected:
			com_ptr<ID3D12RaytracingFallbackCommandList> m_commandListRT;

			const Device* m_pDeviceRT;
		};

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE GetDXRAccelerationStructurePostbuildInfoType(
			PostbuildInfoType type);
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE GetDXRAccelerationStructureCopyMode(
			CopyMode mode);
	}
}
