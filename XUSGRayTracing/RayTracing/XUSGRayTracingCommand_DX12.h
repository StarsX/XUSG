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
				uint32_t numPostbuildInfoDescs,
				const PostbuildInfo* pPostbuildInfoDescs,
				const DescriptorHeap& descriptorHeap) const;

			void SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps) const;
			void SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const;
			void SetTopLevelAccelerationStructure(uint32_t index, uint64_t topLevelASPtr) const;
			void SetRayTracingPipeline(const Pipeline& pipeline) const;
			void DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
				const ShaderTable* pHitGroup, const ShaderTable* pMiss, const ShaderTable* pRayGen) const;
			void DispatchRays(const Pipeline& pipeline, uint32_t width, uint32_t height, uint32_t depth,
				const ShaderTable* pHitGroup, const ShaderTable* pMiss, const ShaderTable* pRayGen) const;

			const Device* GetRTDevice() const;

		protected:
			com_ptr<ID3D12RaytracingFallbackCommandList> m_commandListRT;

			const Device* m_pDeviceRT;
		};
	}
}
