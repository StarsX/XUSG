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

			Buffer::sptr GetResource() const;

			uint32_t GetResultDataMaxSize() const;
			uint32_t GetScratchDataMaxSize() const;
			uint32_t GetUpdateScratchDataSize() const;

			uint64_t GetResourcePointer() const;

			static bool AllocateUAVBuffer(const XUSG::Device* pDevice, Resource* pResource,
				size_t byteWidth, ResourceState dstState = ResourceState::COMMON);
			static bool AllocateUploadBuffer(const XUSG::Device* pDevice, Resource* pResource,
				size_t byteWidth, void* pData);

		protected:
			bool preBuild(const Device* pDevice, uint32_t descriptorIndex, uint32_t numSRVs = 0);

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_buildDesc;
			PrebuildInfo m_prebuildInfo;

			Buffer::sptr m_resource;
			WRAPPED_GPU_POINTER m_pointer;
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
				const DescriptorHeap& descriptorHeap, const BottomLevelAS* pSource = nullptr);

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
				const Resource* pInstanceDescs, const DescriptorHeap& descriptorHeap,
				const TopLevelAS* pSource = nullptr);

			static void SetInstances(const Device* pDevice, Resource* pInstances, uint32_t numInstances,
				const BottomLevelAS* const* ppBottomLevelASs, float* const* transforms);
			static void SetInstances(const Device* pDevice, Resource* pInstances, uint32_t numInstances,
				const InstanceDesc* pInstanceDescs);
		};
	}
}
