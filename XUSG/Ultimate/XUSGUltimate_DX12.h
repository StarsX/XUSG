//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGUltimate.h"

namespace XUSG
{
	namespace Ultimate
	{
		//--------------------------------------------------------------------------------------
		// Command list
		//--------------------------------------------------------------------------------------
		class CommandList_DX12 :
			public virtual CommandList,
			public virtual XUSG::CommandList_DX12
		{
		public:
			CommandList_DX12();
			CommandList_DX12(XUSG::CommandList& commandList);
			~CommandList_DX12();

			bool Create(Device* pDevice, uint32_t nodeMask, CommandListType type,
				uint8_t numAllocators, CommandAllocator* const* ppAllocators, const Pipeline& pipeline,
				const wchar_t* name = nullptr);
			void Create(void* pNativeHandle, Device* pDevice);

			using XUSG::CommandList_DX12::Barrier;
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
			void DispatchMesh(
				uint32_t ThreadGroupCountX,
				uint32_t ThreadGroupCountY,
				uint32_t ThreadGroupCountZ) const;

			void SetStateObject(const Pipeline& stateObject) const;
			void SetProgram(ProgramType type, ProgramIdentifier identifier, WorkGraphFlag flags = WorkGraphFlag::NONE,
				uint64_t backingMemoryAddress = 0, uint64_t backingMemoryByteSize = 0,
				uint64_t localRootArgTableAddress = 0, uint64_t localRootArgTableByteSize = 0,
				uint64_t localRootArgTableByteStride = 0) const;
			void DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride = 0) const;
			void DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes = false) const;

			void ConvertLinearAlgebraMatrix(const LinearAlgebraMatrixConversionInfo* pConversions, uint32_t numConversions) const;

			com_ptr<ID3D12GraphicsCommandList6>& GetGraphicsCommandList();

		protected:
			bool createInterface();
			void createAgilityInterface() const;

			com_ptr<ID3D12GraphicsCommandList6>		m_commandListU;
			mutable com_ptr<ID3D12GraphicsCommandList11>	m_commandListA;

			mutable std::vector<D3D12_BARRIER_GROUP>		m_barrierGroups;
			mutable std::vector<D3D12_GLOBAL_BARRIER>		m_globalBarriers;
			mutable std::vector<D3D12_TEXTURE_BARRIER>		m_textureBarriers;
			mutable std::vector<D3D12_BUFFER_BARRIER>		m_bufferBarriers;
			mutable std::vector<uint32_t>					m_barrierGroupBarrierStarts;

			mutable std::vector<D3D12_NODE_CPU_INPUT>		m_nodeInputs;

			mutable std::vector<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO> m_matrixConversions;
		};

		//--------------------------------------------------------------------------------------
		// Pipeline layout
		//--------------------------------------------------------------------------------------
		class PipelineLayoutLib_DX12 :
			public virtual PipelineLayoutLib,
			public virtual XUSG::PipelineLayoutLib_DX12
		{
		public:
			PipelineLayoutLib_DX12();
			PipelineLayoutLib_DX12(Device* pDevice);
			virtual ~PipelineLayoutLib_DX12();

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

		//--------------------------------------------------------------------------------------
		// Sampler feedback
		//--------------------------------------------------------------------------------------
		class SamplerFeedBack_DX12 :
			public virtual SamplerFeedBack,
			public virtual Texture_DX12
		{
		public:
			SamplerFeedBack_DX12();
			~SamplerFeedBack_DX12();

			using Resource_DX12::Create;
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
			//bool CreateUAV(Resource* pTarget);

			Descriptor CreateUAV(Resource* pTarget);
		};

		ProgramIdentifier GetDX12ProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName);

		void GetDX12LinearAlgebraMatrixInfo(const Device* pDevice, LinearAlgebraMatrixInfo& matrixInfo);

		D3D12_BARRIER_SYNC GetDX12BarrierSync(BarrierSync barrierSync);
		D3D12_BARRIER_SYNC GetDX12BarrierSyncs(BarrierSync barrierSync);

		D3D12_BARRIER_ACCESS GetDX12BarrierAccess(BarrierAccess barrierAccess);
		D3D12_BARRIER_ACCESS GetDX12BarrierAccesses(BarrierAccess barrierAccess);

		D3D12_TEXTURE_BARRIER_FLAGS GetDX12TextureBarrierFlag(TextureBarrierFlag textureBarrierFlag);
		D3D12_TEXTURE_BARRIER_FLAGS GetDX12TextureBarrierFlags(TextureBarrierFlag textureBarrierFlags);

		D3D12_SHADING_RATE_COMBINER GetDX12ShadingRateCombiner(ShadingRateCombiner combiner);
		D3D12_RESOLVE_MODE GetDX12ResolveMode(ResolveMode mode);

		D3D12_VIEW_INSTANCING_FLAGS GetDX12ViewInstanceFlag(ViewInstanceFlag viewInstanceFlag);
		D3D12_VIEW_INSTANCING_FLAGS GetDX12ViewInstanceFlags(ViewInstanceFlag viewInstanceFlags);

		D3D12_SET_WORK_GRAPH_FLAGS GetDX12WorkGraphFlag(WorkGraphFlag workGraphFlag);
		D3D12_SET_WORK_GRAPH_FLAGS GetDX12WorkGraphFlags(WorkGraphFlag workGraphFlags);

		D3D12_LINEAR_ALGEBRA_MATRIX_LAYOUT GetDX12LinearAlgebraMatrixLayout(LinearAlgebraMatrixLayout layout);
		D3D12_LINEAR_ALGEBRA_DATATYPE GetDX12LinearAlgebraDataType(LinearAlgebraDataType dataType);
	}
}
