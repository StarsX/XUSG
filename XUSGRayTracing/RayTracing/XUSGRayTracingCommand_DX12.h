//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Ultimate/XUSGUltimate_DX12.h"
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
			CommandList_DX12(XUSG::CommandList* pCommandList, Device* pDevice);
			virtual ~CommandList_DX12();

			bool Create(XUSG::Device* pDevice, uint32_t nodeMask, CommandListType type,
				uint8_t numAllocators, CommandAllocator* const* ppAllocators, const Pipeline& pipeline,
				const wchar_t* name = nullptr);
			bool Create(Device* pDevice, uint32_t nodeMask, CommandListType type,
				uint8_t numAllocators, CommandAllocator* const* ppAllocators, const Pipeline& pipeline,
				const wchar_t* name = nullptr);
			void Create(void* pNativeHandle, Device* pDevice);

			void Barrier(uint32_t numBarriers, const ResourceBarrier* pBarriers);

			void BuildRaytracingAccelerationStructure(const BuildDesc* pDesc,
				uint32_t numPostbuildInfoDescs, const PostbuildInfo* pPostbuildInfoDescs,
				const DescriptorHeap* pDescriptorHeap = nullptr);
			void EmitRaytracingAccelerationStructurePostbuildInfo(const PostbuildInfo* pDesc,
				uint32_t numAccelerationStructures, const uint64_t* pAccelerationStructureData);
			void CopyRaytracingAccelerationStructure(AccelerationStructure* pDst,
				AccelerationStructure* pSrc, CopyMode mode,
				const DescriptorHeap* pDescriptorHeap = nullptr);

			void SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps);
			void SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const;
			void SetTopLevelAccelerationStructure(uint32_t index, uint64_t topLevelASPtr) const;
			void SetRayTracingPipeline(const Pipeline& pipeline) const;
			void DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
				const ShaderTable* pRayGen, const ShaderTable* pHitGroup, const ShaderTable* pMiss,
				const ShaderTable* pCallable = nullptr) const;

			Device* GetRTDevice() const;
			void* GetRTBackend();

		protected:
			bool createInterface();

			com_ptr<ID3D12RaytracingFallbackCommandList> m_commandListRT;

			Device* m_pDeviceRT;
		};

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE GetDXRAccelerationStructurePostbuildInfoType(PostbuildInfoType type);
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE GetDXRAccelerationStructureCopyMode(CopyMode mode);
	}
}
