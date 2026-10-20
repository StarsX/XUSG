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

			bool Create(Device* pDevice, uint32_t nodeMask, CommandListType type,
				uint8_t numAllocators, CommandAllocator* const* ppAllocators, const Pipeline& pipeline,
				const wchar_t* name = nullptr);
			bool CreateInterface();
			void AttachUltimate(const CommandList_VK& source);

			using XUSG::CommandList_VK::Barrier;
			void Barrier(uint32_t numBarrierGroups, BarrierGroup* pBarrierGroups) const;
			void Barrier(uint32_t numBufferBarriers, ResourceBarrier* pBufferBarriers,
				uint32_t numTextureBarriers, ResourceBarrier* pTextureBarriers,
				uint32_t numGlobalBarriers, ResourceBarrier* pGlobalBarriers) const;
			void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const;
			void ResolveSubresourceRegion(Resource* pDstResource, uint32_t dstSubresource,
				uint32_t dstX, uint32_t dstY, Resource* pSrcResource, uint32_t srcSubresource,
				const RectRange& srcRect, Format format, ResolveMode resolveMode) const;

			void RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const;
			void RSSetShadingRateImage(Resource* pShadingRateImage) const;
			void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) const;

			void SetStateObject(const Pipeline& stateObject) const;
			void SetProgram(ProgramType type, ProgramIdentifier identifier, WorkGraphFlag flags = WorkGraphFlag::NONE,
				uint64_t backingMemoryAddress = 0, uint64_t backingMemoryByteSize = 0,
				uint64_t localRootArgTableAddress = 0, uint64_t localRootArgTableByteSize = 0,
				uint64_t localRootArgTableByteStride = 0) const;
			void DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride = 0) const;
			void DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes = false) const;

			void ConvertLinearAlgebraMatrix(const LinearAlgebraMatrixConversionInfo* pConversions, uint32_t numConversions) const;

		protected:
			struct CachedView
			{
				VkImage Image;
				VkFormat Format;
				VkImageViewType Type;
				uint32_t Mip;
				uint32_t Layer;
				VkImageAspectFlags Aspect;
				VkImageView View;
			};

			const void* renderingPNext() const;
			void loadProcs();
			void flushBarriers(VkMemoryBarrier2& memory) const;
			void addBarrier(const ResourceBarrier& barrier, VkMemoryBarrier2& memory) const;
			bool ultimateBarriersTouchBoundAttachment(uint32_t numBarriers, const ResourceBarrier* pBarriers) const;
			void prepareUltimateBarriersOutsidePass(uint32_t numBarriers, const ResourceBarrier* pBarriers) const;
			void resolveMinMax(Resource* pDstResource, uint32_t dstSubresource, uint32_t dstX, uint32_t dstY,
				Resource* pSrcResource, uint32_t srcSubresource, const RectRange& srcRect,
				ResolveMode resolveMode) const;
			bool viewCached(const CachedView& cached, const XUSG::Resource_VK* pResource, uint32_t mip,
				uint32_t layer, VkImageAspectFlags aspect, VkImageViewType type) const;
			bool cachedView(CachedView& cached, const XUSG::Resource_VK* pResource, uint32_t mip,
				uint32_t layer, VkImageAspectFlags aspect, VkImageViewType type) const;

			struct UltimateState
			{
				~UltimateState();

				Device_VK*								m_pDevice = nullptr;
				PFN_vkCmdSetFragmentShadingRateKHR		m_setShadingRate = nullptr;
				PFN_vkCmdDrawMeshTasksEXT				m_drawMesh = nullptr;
				CachedView	m_shadingRate = {};
				CachedView	m_resolveSrc = {};
				CachedView	m_resolveDst = {};
				VkRenderingFragmentShadingRateAttachmentInfoKHR m_shadingRateInfo = {};
			};

			std::shared_ptr<UltimateState> m_ultimate;
		};

		class PipelineLayoutLib_VK :
			public XUSG::PipelineLayoutLib_VK,
			public virtual PipelineLayoutLib
		{
		public:
			PipelineLayoutLib_VK();
			PipelineLayoutLib_VK(Device* pDevice);
			virtual ~PipelineLayoutLib_VK();

			PipelineLayout CreateRootSignatureFromLibSubobject(const Blob& blobLib,
				const wchar_t* name, uint32_t nodeMask = 0);
			PipelineLayout GetRootSignatureFromLibSubobject(const Blob& blobLib,
				const wchar_t* name, bool create = true, uint32_t nodeMask = 0);

		protected:
			virtual PipelineLayout createRootSignatureFromLibSubobject(const std::string& key,
				const Blob& blobLib, const wchar_t* name, uint32_t nodeMask);
			PipelineLayout getRootSignatureFromLibSubobject(const std::string& key, const Blob& blobLib,
				const wchar_t* name, bool create, uint32_t nodeMask);
		};

		class SamplerFeedBack_VK :
			public virtual SamplerFeedBack,
			public virtual Texture_VK
		{
		public:
			SamplerFeedBack_VK();
			~SamplerFeedBack_VK();

			using Resource_VK::Create;
			bool Create(Device* pDevice, Texture* pTarget, Format format, uint32_t mipRegionWidth,
				uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags = ResourceFlag::NONE,
				bool isCubeMap = false, MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
				uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
				TextureLayout textureLayout = TextureLayout::UNKNOWN,
				uint32_t maxThreads = 1);
			bool Create(Device* pDevice, Heap* pHeap, uint64_t heapOffset, Texture* pTarget,
				Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
				ResourceFlag resourceFlags = ResourceFlag::NONE, bool isCubeMap = false,
				const wchar_t* name = nullptr, uint16_t srvComponentMapping = XUSG_DEFAULT_SRV_COMPONENT_MAPPING,
				TextureLayout textureLayout = TextureLayout::UNKNOWN,
				uint32_t maxThreads = 1);
			bool CreateResource(Device* pDevice, Texture* pTarget, Format format, uint32_t mipRegionWidth,
				uint32_t mipRegionHeight, uint32_t mipRegionDepth, ResourceFlag resourceFlags = ResourceFlag::NONE,
				bool isCubeMap = false, MemoryFlag memoryFlags = MemoryFlag::NONE,
				ResourceState initialResourceState = ResourceState::COMMON,
				TextureLayout textureLayout = TextureLayout::UNKNOWN,
				uint32_t maxThreads = 1);
			bool CreateResource(Device* pDevice, Heap* pHeap, uint64_t heapOffset, Texture* pTarget,
				Format format, uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
				ResourceFlag resourceFlags = ResourceFlag::NONE, bool isCubeMap = false,
				ResourceState initialResourceState = ResourceState::COMMON,
				TextureLayout textureLayout = TextureLayout::UNKNOWN,
				uint32_t maxThreads = 1);

			Descriptor CreateUAV(Resource* pTarget);
		};
	}
}
