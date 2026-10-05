//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSGCommand_VK.h"
#include "Core/XUSGResource_VK.h"
#include "Core/XUSGPipelineLayout_VK.h"
#include "XUSGUltimate.h"

namespace XUSG
{
	namespace Ultimate
	{
		class CommandList_VK :
			public virtual CommandList,
			public virtual XUSG::CommandList_VK
		{
		public:
			CommandList_VK();
			CommandList_VK(XUSG::CommandList& commandList);
			virtual ~CommandList_VK();

			bool Create(const Device* pDevice, uint32_t nodeMask, CommandListType type,
				const CommandAllocator* pAllocator, const Pipeline& pipeline,
				const wchar_t* name = nullptr);
			bool CreateInterface();

			using XUSG::CommandList_VK::Barrier;
			void Barrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups);
			void Barrier(uint32_t numBufferBarriers, ResourceBarrier* pBufferBarriers,
				uint32_t numTextureBarriers, ResourceBarrier* pTextureBarriers,
				uint32_t numGlobalBarriers, ResourceBarrier* pGlobalBarriers);
			void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const;
			void ResolveSubresourceRegion(const Resource* pDstResource, uint32_t dstSubresource,
				uint32_t dstX, uint32_t dstY, const Resource* pSrcResource, uint32_t srcSubresource,
				const RectRange& srcRect, Format format, ResolveMode resolveMode) const;

			void RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const;
			void RSSetShadingRateImage(const Resource* pShadingRateImage) const;
			void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) const;

			void SetStateObject(const Pipeline& stateObject);
			void SetProgram(ProgramType type, ProgramIdentifier identifier, WorkGraphFlag flags = WorkGraphFlag::NONE,
				uint64_t backingMemoryAddress = 0, uint64_t backingMemoryByteSize = 0,
				uint64_t localRootArgTableAddress = 0, uint64_t localRootArgTableByteSize = 0,
				uint64_t localRootArgTableByteStride = 0);
			void DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride = 0);
			void DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes = false);

			void ConvertLinearAlgebraMatrix(const LinearAlgebraMatrixConversionInfo* pConversions, uint32_t numConversions);

		protected:
			const void* renderingPNext() const;
			void loadProcs();

			PFN_vkCmdSetFragmentShadingRateKHR	m_setShadingRate;
			PFN_vkCmdDrawMeshTasksEXT			m_drawMesh;

			mutable VkImageView m_shadingRateView;
			mutable VkRenderingFragmentShadingRateAttachmentInfoKHR m_shadingRateInfo;
		};

		class PipelineLayoutLib_VK :
			public XUSG::PipelineLayoutLib_VK,
			public virtual PipelineLayoutLib
		{
		public:
			PipelineLayoutLib_VK();
			PipelineLayoutLib_VK(const Device* pDevice);
			virtual ~PipelineLayoutLib_VK();

			PipelineLayout CreateRootSignatureFromLibSubobject(const Blob& blobLib,
				const wchar_t* name, uint32_t nodeMask = 0);
			PipelineLayout GetRootSignatureFromLibSubobject(const Blob& blobLib,
				const wchar_t* name, bool create = true, uint32_t nodeMask = 0);
		};

		class SamplerFeedBack_VK :
			public virtual SamplerFeedBack,
			public Texture_VK
		{
		public:
			SamplerFeedBack_VK();
			virtual ~SamplerFeedBack_VK();

			using Resource_VK::Create;
			bool Create(const Device* pDevice, Texture* pTarget, Format format, uint32_t mipRegionWidth,
				uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags = ResourceFlag::NONE,
				bool isCubeMap = false, MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
				uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
				TextureLayout textureLayout = TextureLayout::UNKNOWN,
				uint32_t maxThreads = 1);
			bool Create(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, Texture* pTarget,
				Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
				ResourceFlag resourceFlags = ResourceFlag::NONE, bool isCubeMap = false,
				const wchar_t* name = nullptr, uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
				TextureLayout textureLayout = TextureLayout::UNKNOWN,
				uint32_t maxThreads = 1);
			bool CreateResource(const Device* pDevice, Texture* pTarget, Format format, uint32_t mipRegionWidth,
				uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags = ResourceFlag::NONE,
				bool isCubeMap = false, MemoryFlag memoryFlags = MemoryFlag::NONE,
				ResourceState initialResourceState = ResourceState::COMMON,
				TextureLayout textureLayout = TextureLayout::UNKNOWN,
				uint32_t maxThreads = 1);
			bool CreateResource(const Device* pDevice, const Heap* pHeap, uint64_t heapOffset, Texture* pTarget,
				Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
				ResourceFlag resourceFlags = ResourceFlag::NONE, bool isCubeMap = false,
				ResourceState initialResourceState = ResourceState::COMMON,
				TextureLayout textureLayout = TextureLayout::UNKNOWN,
				uint32_t maxThreads = 1);

			Descriptor CreateUAV(Resource* pTarget);
		};
	}
}
