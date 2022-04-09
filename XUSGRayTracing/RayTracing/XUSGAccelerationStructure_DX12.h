//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		class AccelerationStructure_DX12 :
			public virtual AccelerationStructure
		{
		public:
			AccelerationStructure_DX12();
			virtual ~AccelerationStructure_DX12();

			RawBuffer::sptr GetResult() const;

			uint32_t GetResultDataMaxSize() const;
			uint32_t GetScratchDataMaxSize() const;
			uint32_t GetUpdateScratchDataSize() const;
#if XUSG_ENABLE_DXR_FALLBACK
			const WRAPPED_GPU_POINTER& GetResultPointer() const;
#endif

			static bool AllocateUAVBuffer(const XUSG::Device* pDevice, Resource* pResource,
				size_t byteWidth, ResourceState dstState = ResourceState::UNORDERED_ACCESS);
			static bool AllocateUploadBuffer(const XUSG::Device* pDevice, Resource* pResource,
				size_t byteWidth, void* pData);

		protected:
			bool preBuild(const Device* pDevice, uint32_t descriptorIndex, uint32_t numSRVs = 0);

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_buildDesc;
			PrebuildInfo m_prebuildInfo;

			std::vector<RawBuffer::sptr> m_results;
#if XUSG_ENABLE_DXR_FALLBACK
			std::vector<WRAPPED_GPU_POINTER> m_pointers;
#endif

			uint32_t m_currentFrame;
		};

		class BottomLevelAS_DX12 :
			public virtual BottomLevelAS,
			public virtual AccelerationStructure_DX12
		{
		public:
			BottomLevelAS_DX12();
			virtual ~BottomLevelAS_DX12();

			bool PreBuild(const Device* pDevice, uint32_t numGeometries, const GeometryBuffer& geometries,
				uint32_t descriptorIndex, BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);
			void Build(CommandList* pCommandList, const Resource* pScratch,
				const DescriptorPool& descriptorPool, bool update = false);

			static void SetTriangleGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
				Format vertexFormat, const VertexBufferView* pVBs, const IndexBufferView* pIBs = nullptr,
				const GeometryFlag* pGeometryFlags = nullptr, const ResourceView* pTransforms = nullptr);
			static void SetAABBGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
				const VertexBufferView* pVBs, const GeometryFlag* pGeometryFlags = nullptr);
		};

		class TopLevelAS_DX12 :
			public TopLevelAS,
			public AccelerationStructure_DX12
		{
		public:
			TopLevelAS_DX12();
			virtual ~TopLevelAS_DX12();

			bool PreBuild(const Device* pDevice, uint32_t numInstances, uint32_t descriptorIndex,
				BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);
			void Build(const CommandList* pCommandList, const Resource* pScratch,
				const Resource* pInstanceDescs, const DescriptorPool& descriptorPool, bool update = false);
#if !XUSG_ENABLE_DXR_FALLBACK
			void Build(XUSG::CommandList* pCommandList, const Resource* pScratch,
				const Resource* pInstanceDescs, const DescriptorPool& descriptorPool, bool update = false);
#endif

			static void SetInstances(const Device* pDevice, Resource* pInstances, uint32_t numInstances,
				const BottomLevelAS* const* ppBottomLevelASs, float* const* transforms);
			static void SetInstances(const Device* pDevice, Resource* pInstances, uint32_t numInstances,
				const InstanceDesc* pInstanceDescs);
		};
	}
}
