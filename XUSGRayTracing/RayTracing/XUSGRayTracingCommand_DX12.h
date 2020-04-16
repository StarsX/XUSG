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
			public virtual XUSG::CommandList_DX12
		{
		public:
			CommandList_DX12();
			virtual ~CommandList_DX12();

			bool CreateRaytracingInterfaces(const Device& device);

			void BuildRaytracingAccelerationStructure(const BuildDesc* pDesc,
				uint32_t numPostbuildInfoDescs,
				const PostbuildInfo* pPostbuildInfoDescs,
				const DescriptorPool& descriptorPool) const;

			void SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const;
			void SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS& topLevelAS) const;
			void DispatchRays(const Pipeline& pipeline, uint32_t width, uint32_t height, uint32_t depth,
				const ShaderTable& hitGroup, const ShaderTable& miss, const ShaderTable& rayGen) const;

		protected:
#if ENABLE_DXR_FALLBACK
			FallbackCommandList m_fallback;
#endif
			NativeCommandList m_native;

#if ENABLE_DXR_FALLBACK
			API m_raytracingAPI;
#endif
		};
	}
}