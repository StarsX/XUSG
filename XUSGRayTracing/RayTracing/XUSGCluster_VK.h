//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		class ClusterLevelAS_VK :
			public virtual ClusterLevelAS
		{
		public:
			ClusterLevelAS_VK();
			virtual ~ClusterLevelAS_VK();

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

			uintptr_t m_byteOffset;
		};

		class ClusterTemplate_VK :
			public virtual ClusterTemplate
		{
		public:
			ClusterTemplate_VK();
			virtual ~ClusterTemplate_VK();

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

			uintptr_t m_byteOffset;
		};

		class ClusterBottomLevelAS_VK :
			public virtual ClusterBottomLevelAS
		{
		public:
			ClusterBottomLevelAS_VK();
			virtual ~ClusterBottomLevelAS_VK();

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

			uintptr_t m_byteOffset;
		};

		class PartitionedTLAS_VK :
			public virtual PartitionedTLAS
		{
		public:
			PartitionedTLAS_VK();
			virtual ~PartitionedTLAS_VK();

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
			RTASPrebuildInfo m_prebuildInfo;

			Descriptor m_srv;

			uintptr_t m_byteOffset;
		};
	}
}
