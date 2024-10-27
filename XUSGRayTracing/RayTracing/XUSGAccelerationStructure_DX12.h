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

			void SetDestination(const Device* pDevice, const Buffer::sptr destBuffer, uintptr_t byteOffsett,
				uint32_t uavIndex, DescriptorTableLib* pDescriptorTableLib);

			uint32_t SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers = 0);

			const PrebuildInfo& GetPrebuildInfo() const;

			Buffer* GetPostbuildInfo() const;

			size_t GetResultDataMaxByteSize(bool isAligned = true) const;
			size_t GetScratchDataByteSize() const;
			size_t GetUpdateScratchDataByteSize() const;
			size_t GetCompactedByteSize(bool isAligned = true) const;

			uint64_t GetVirtualAddress() const;
			uint64_t GetResourcePointer() const;

			static bool AllocateDestBuffer(const Device* pDevice, Buffer* pDestBuffer, size_t byteWidth,
				uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
				uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
				uint32_t maxThreads = 1);

			static size_t Align(size_t byteSize);

		protected:
			bool prebuild(const Device* pDevice);
			bool allocate(const Device* pDevice, size_t byteWidth, DescriptorTableLib* pDescriptorTableLib,
				uint32_t numSRVs, MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads);

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_buildDesc;
			PrebuildInfo m_prebuildInfo;

			Buffer::sptr m_resource;
			Buffer::sptr m_postbuildInfoRB;
			Buffer::uptr m_postbuildInfo;

			WRAPPED_GPU_POINTER m_pointer;

			uintptr_t m_byteOffset;
		};

		class BottomLevelAS_DX12 :
			public virtual BottomLevelAS,
			public virtual AccelerationStructure_DX12
		{
		public:
			BottomLevelAS_DX12();
			virtual ~BottomLevelAS_DX12();

			bool Prebuild(const Device* pDevice, uint32_t numGeometries, const GeometryBuffer& geometries,
				BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);
			bool Allocate(const Device* pDevice, DescriptorTableLib* pDescriptorTableLib, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1);

			void Build(CommandList* pCommandList, const Resource* pScratch,
				const BottomLevelAS* pSource = nullptr, uint8_t numPostbuildInfoDescs = 0,
				const PostbuildInfoType* pPostbuildInfoTypes = nullptr);

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

			bool Prebuild(const Device* pDevice, uint32_t numInstances, BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);
			bool Allocate(const Device* pDevice, DescriptorTableLib* pDescriptorTableLib, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1);

			void SetDestination(const Device* pDevice, const Buffer::sptr destBuffer, uintptr_t byteOffset,
				uint32_t uavIndex, uint32_t srvIndex, DescriptorTableLib* pDescriptorTableLib);
			void Build(CommandList* pCommandList, const Resource* pScratch,
				const Resource* pInstanceDescs, const DescriptorHeap& descriptorHeap,
				const TopLevelAS* pSource = nullptr, uint8_t numPostbuildInfoDescs = 0,
				const PostbuildInfoType* pPostbuildInfoTypes = nullptr);

			const Descriptor& GetSRV() const;

			static void SetInstances(const Device* pDevice, Buffer* pInstances, uint32_t numInstances,
				const InstanceDesc* pInstanceDescs, MemoryFlag memoryFlags = MemoryFlag::NONE,
				const wchar_t* instanceName = nullptr);

		protected:
			uint32_t m_srvIndex;
		};
	}
}
