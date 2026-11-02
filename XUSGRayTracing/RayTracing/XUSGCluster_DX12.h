//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		class ClusterLevelAS_DX12 :
			public virtual ClusterLevelAS
		{
		public:
			ClusterLevelAS_DX12();
			virtual ~ClusterLevelAS_DX12();

			void SetTriangles(const RTASClusterTrianglesInputs& desc);
			void SetTemplateInstantiation(const RTASInstantiateClusterTemplateInputs& desc);

			bool Prebuild(Device* pDevice);
			bool Allocate(Device* pDevice, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1);
			void Build(CommandList* pCommandList, const Resource* pScratch,
				uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride = 0);

			uint32_t SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers = 0);

			const RTASPrebuildInfo& GetPrebuildInfo() const;

			size_t GetResultDataMaxByteSize(bool isAligned = true) const;
			size_t GetScratchDataByteSize(bool isAligned = true) const;

			uint64_t GetVirtualAddress() const;

			void* GetBackend();

			static size_t Align(size_t byteSize);

		protected:
			RTASPrebuildInfo m_prebuildInfo;

			RTASClusterTrianglesInputs m_triangles;

			RTASInstantiateClusterTemplateInputs m_instantiate;

			Buffer::sptr m_resource;

			uintptr_t m_byteOffset;

			RTASOperationType m_type;
		};

		class ClusterTemplate_DX12 :
			public virtual ClusterTemplate
		{
		public:
			ClusterTemplate_DX12();
			virtual ~ClusterTemplate_DX12();

			void SetTriangles(const RTASClusterTemplateTrianglesInputs& desc);

			bool Prebuild(Device* pDevice);
			bool Allocate(Device* pDevice, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1);
			void Build(CommandList* pCommandList, const Resource* pScratch,
				uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride = 0);
			void Instantiate(CommandList* pCommandList, ClusterLevelAS* pDestination, const Resource* pScratch,
				uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride = 0);

			uint32_t SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers = 0);

			const RTASPrebuildInfo& GetPrebuildInfo() const;

			size_t GetResultDataMaxByteSize(bool isAligned = true) const;
			size_t GetScratchDataByteSize(bool isAligned = true) const;

			uint64_t GetVirtualAddress() const;

			void* GetBackend();

			static size_t Align(size_t byteSize);

		protected:
			RTASPrebuildInfo m_prebuildInfo;

			RTASClusterTemplateTrianglesInputs m_triangles;

			Buffer::sptr m_resource;

			uintptr_t m_byteOffset;
		};

		class ClusterBottomLevelAS_DX12 :
			public virtual ClusterBottomLevelAS
		{
		public:
			ClusterBottomLevelAS_DX12();
			virtual ~ClusterBottomLevelAS_DX12();

			void SetClusters(const RTASClasInputs& desc);

			bool Prebuild(Device* pDevice);
			bool Allocate(Device* pDevice, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1);
			void Build(CommandList* pCommandList, const Resource* pScratch,
				uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride = 0);

			uint32_t SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers = 0);

			const RTASPrebuildInfo& GetPrebuildInfo() const;

			size_t GetResultDataMaxByteSize(bool isAligned = true) const;
			size_t GetScratchDataByteSize(bool isAligned = true) const;

			uint64_t GetVirtualAddress() const;

			void* GetBackend();

			static size_t Align(size_t byteSize);

		protected:
			RTASPrebuildInfo m_prebuildInfo;

			RTASClasInputs m_clusters;

			Buffer::sptr m_resource;

			uintptr_t m_byteOffset;
		};

		class PartitionedTLAS_DX12 :
			public virtual PartitionedTLAS
		{
		public:
			PartitionedTLAS_DX12();
			virtual ~PartitionedTLAS_DX12();

			void SetPartitions(const RTASPartitionedTLASInputs& desc);

			bool Prebuild(Device* pDevice);
			bool Allocate(Device* pDevice, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1);
			void Build(CommandList* pCommandList, const Resource* pScratch,
				uint64_t indirectOps, uint64_t indirectOpCount);
			void Update(CommandList* pCommandList, const Resource* pScratch,
				uint64_t indirectOps, uint64_t indirectOpCount);

			uint32_t SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers = 0);

			const RTASPrebuildInfo& GetPrebuildInfo() const;

			size_t GetResultDataMaxByteSize(bool isAligned = true) const;
			size_t GetScratchDataByteSize(bool isAligned = true) const;

			uint64_t GetVirtualAddress() const;
			const Descriptor& GetSRV() const;

			void* GetBackend();

			static size_t Align(size_t byteSize);

		protected:
			void build(CommandList* pCommandList, const Resource* pScratch,
				uint64_t indirectOps, uint64_t indirectOpCount, uint64_t source);

			RTASPrebuildInfo m_prebuildInfo;

			RTASPartitionedTLASInputs m_partitions;

			Buffer::sptr m_resource;

			uintptr_t m_byteOffset;
		};
	}
}
