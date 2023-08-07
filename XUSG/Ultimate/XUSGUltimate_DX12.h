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

			bool CreateInterface();

			void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const;
			void ResolveSubresourceRegion(const Resource* pDstResource, uint32_t dstSubresource,
				uint32_t dstX, uint32_t dstY, const Resource* pSrcResource, uint32_t srcSubresource,
				const RectRange& srcRect, Format format, ResolveMode resolveMode) const;

			void RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const;
			void RSSetShadingRateImage(const Resource* pShadingRateImage) const;
			void DispatchMesh(
				uint32_t ThreadGroupCountX,
				uint32_t ThreadGroupCountY,
				uint32_t ThreadGroupCountZ) const;

			void SetProgram(ProgramType type, ProgramIdentifier identifier, WorkGraphFlag flags,
				uint64_t backingMemoryAddress = 0, uint64_t backingMemoryByteSize = 0,
				uint64_t localRootArgTableAddress = 0, uint64_t localRootArgTableByteSize = 0,
				uint64_t localRootArgTableByteStride = 0) const;
			void DispatchGraph(uint32_t numNodeInputs, const NodeCPUInput* pNodeInputs, uint64_t nodeInputByteStride = 0) const;
			void DispatchGraph(uint64_t nodeGPUInputAddress, bool isMultiNodes = false) const;

			com_ptr<ID3D12GraphicsCommandList6>& GetGraphicsCommandList();

		protected:
			com_ptr<ID3D12GraphicsCommandList6> m_commandListU;
		};

		ProgramIdentifier GetProgramIdentifierFromDX12(const Pipeline& stateObject, const wchar_t* programName);

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

			bool Create(const Device* pDevice, const Texture* pTarget, Format format,
				uint32_t mipRegionWidth, uint32_t mipRegionHeight, uint32_t mipRegionDepth,
				ResourceFlag resourceFlags = ResourceFlag::NONE, bool isCubeMap = false,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
				uint32_t maxThreads = 1);
			bool CreateUAV(const Resource* pTarget);

		protected:
			com_ptr<ID3D12Device8> m_deviceU;
		};

		D3D12_SHADING_RATE_COMBINER GetDX12ShadingRateCombiner(ShadingRateCombiner combiner);
		D3D12_RESOLVE_MODE GetDX12ResolveMode(ResolveMode mode);

		D3D12_SET_WORK_GRAPH_FLAGS GetDX12WorkGraphFlag(WorkGraphFlag workGraphFlag);
		D3D12_SET_WORK_GRAPH_FLAGS GetDX12WorkGraphFlags(WorkGraphFlag workGraphFlags);
	}
}
