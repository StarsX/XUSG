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
#if ENABLE_DXR_FALLBACK
			const WRAPPED_GPU_POINTER& GetResultPointer() const;
#endif

			static bool AllocateUAVBuffer(const Device& device, Resource& resource,
				size_t byteWidth, ResourceState dstState = ResourceState::UNORDERED_ACCESS);
			static bool AllocateUploadBuffer(const Device& device, Resource& resource,
				size_t byteWidth, void* pData);

		protected:
			bool preBuild(const RayTracing::Device& device, uint32_t descriptorIndex, uint32_t numSRVs = 0);

			BuildDesc		m_buildDesc;
			PrebuildInfo	m_prebuildInfo;

			std::vector<RawBuffer::sptr> m_results;
#if ENABLE_DXR_FALLBACK
			std::vector<WRAPPED_GPU_POINTER> m_pointers;
#endif

			uint32_t		m_currentFrame;
		};

		class BottomLevelAS_DX12 :
			public virtual BottomLevelAS,
			public virtual AccelerationStructure_DX12
		{
		public:
			BottomLevelAS_DX12();
			virtual ~BottomLevelAS_DX12();

			bool PreBuild(const RayTracing::Device& device, uint32_t numDescs, const Geometry* pGeometries,
				uint32_t descriptorIndex, BuildFlags flags = BuildFlags::PREFER_FAST_TRACE);
			void Build(const RayTracing::CommandList* pCommandList, const Resource& scratch,
				const DescriptorPool& descriptorPool, bool update = false);

			static void SetTriangleGeometries(Geometry* pGeometries, uint32_t numGeometries,
				Format vertexFormat, const VertexBufferView* pVBs, const IndexBufferView* pIBs = nullptr,
				const GeometryFlags* pGeometryFlags = nullptr, const ResourceView* pTransforms = nullptr);
			static void SetAABBGeometries(Geometry* pGeometries, uint32_t numGeometries,
				const VertexBufferView* pVBs, const GeometryFlags* pGeometryFlags = nullptr);
		};

		class TopLevelAS_DX12 :
			public TopLevelAS,
			public AccelerationStructure_DX12
		{
		public:
			TopLevelAS_DX12();
			virtual ~TopLevelAS_DX12();

			bool PreBuild(const RayTracing::Device& device, uint32_t numDescs, uint32_t descriptorIndex,
				BuildFlags flags = BuildFlags::PREFER_FAST_TRACE);
			void Build(const RayTracing::CommandList* pCommandList, const Resource& scratch,
				const Resource& instanceDescs, const DescriptorPool& descriptorPool, bool update = false);

			static void SetInstances(const RayTracing::Device& device, Resource& instances,
				uint32_t numInstances, const BottomLevelAS* const* ppBottomLevelASs, float* const* transforms);
		};
	}
}
