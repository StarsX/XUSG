//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Ultimate/XUSGUltimate_VK.h"
#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		class CommandList_VK :
			public virtual CommandList,
			public virtual Ultimate::CommandList_VK
		{
		public:
			CommandList_VK();
			CommandList_VK(XUSG::CommandList* pCommandList, Device* pDevice);
			virtual ~CommandList_VK();

			bool CreateInterface();
			void AttachRayTracing(const CommandList_VK& source);

			bool Reset(uint8_t allocatorIndex, const Pipeline& initialState) const;

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

			Device* GetRTDevice() const;

		protected:
			struct QueryPool
			{
				VkQueryPool Pool;
				VkQueryType Type;
				uint32_t Count;
				uint32_t Used;
			};

			struct QueryAllocation
			{
				VkQueryPool Pool;
				uint32_t First;
			};

			QueryAllocation allocateQueries(VkQueryType type, uint32_t count) const;

			struct RTState
			{
				~RTState();

				XUSG::Device_VK* m_pDevice = nullptr;
				PFN_vkCmdBuildAccelerationStructuresKHR m_cmdBuildAccelerationStructures = nullptr;
				PFN_vkCmdCopyAccelerationStructureKHR m_cmdCopyAccelerationStructure = nullptr;
				PFN_vkCmdWriteAccelerationStructuresPropertiesKHR m_cmdWriteAccelerationStructuresProperties = nullptr;
				PFN_vkCmdBuildMicromapsEXT m_cmdBuildMicromaps = nullptr;
				PFN_vkCmdCopyMicromapEXT m_cmdCopyMicromap = nullptr;
				PFN_vkCmdWriteMicromapsPropertiesEXT m_cmdWriteMicromapsProperties = nullptr;
				PFN_vkCmdTraceRaysKHR m_cmdTraceRays = nullptr;
				std::vector<QueryPool> m_queryPools;
			};

			std::shared_ptr<RTState> m_rt;
			Device* m_pDeviceRT;
		};

		VkQueryType GetVKAccelerationStructurePostbuildQueryType(PostbuildInfoType type);
		VkCopyAccelerationStructureModeKHR GetVKAccelerationStructureCopyMode(CopyMode mode);
	}
}
