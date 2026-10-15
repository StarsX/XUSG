//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG_VK.h"
#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		struct BuildDesc_VK
		{
			VkAccelerationStructureBuildGeometryInfoKHR Info;
			std::vector<VkAccelerationStructureGeometryKHR> Geometries;
			std::vector<VkAccelerationStructureBuildRangeInfoKHR> Ranges;
			std::vector<uint32_t> PrimitiveCounts;
			std::vector<VkAccelerationStructureTrianglesOpacityMicromapKHR> OpacityMicromaps;
			std::vector<VkAccelerationStructureTrianglesOpacityMicromapEXT> OpacityMicromapsExt;
			std::vector<VkMicromapUsageKHR> Usages;
			std::vector<VkMicromapUsageEXT> UsagesExt;
			VkAccelerationStructureGeometryMicromapDataKHR MicromapData;
			VkMicromapBuildInfoEXT MicromapInfo;
			VkAccelerationStructureKHR AccelerationStructure;
			VkMicromapEXT Micromap;
			VkAccelerationStructureTypeKHR Type;
			VkBuildAccelerationStructureFlagsKHR Flags;
		};

		void RegisterVKBuffer(VkBuffer buffer, VkDeviceAddress address, VkDeviceSize size);
		void UnregisterVKBuffer(VkBuffer buffer);
		bool FindVKBuffer(VkDeviceAddress address, VkBuffer& buffer, VkDeviceSize& offset);

		void RegisterVKAccelerationStructure(VkAccelerationStructureKHR accelerationStructure, VkDeviceAddress address);
		void UnregisterVKAccelerationStructure(VkAccelerationStructureKHR accelerationStructure);
		VkAccelerationStructureKHR FindVKAccelerationStructure(uint64_t value);

		void RegisterVKMicromap(VkMicromapEXT micromap);
		void UnregisterVKMicromap(VkMicromapEXT micromap);
		VkMicromapEXT FindVKMicromap(uint64_t value);

		class AccelerationStructure_VK :
			public virtual AccelerationStructure
		{
		public:
			AccelerationStructure_VK();
			virtual ~AccelerationStructure_VK();

			void SetDestination(Device* pDevice, const Buffer::sptr destBuffer, uintptr_t byteOffset,
				uint32_t uavIndex, DescriptorTableLib* pDescriptorTableLib);

			uint32_t SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers = 0);

			const PrebuildInfo& GetPrebuildInfo() const;

			Buffer* GetPostbuildInfo() const;

			size_t GetResultDataMaxByteSize(bool isAligned = true) const;
			size_t GetScratchDataByteSize(bool isAligned = true) const;
			size_t GetUpdateScratchDataByteSize(bool isAligned = true) const;
			size_t GetCompactedByteSize(bool isAligned = true) const;

			uint64_t GetVirtualAddress() const;
			uint64_t GetResourcePointer() const;

			VkAccelerationStructureKHR GetHandle() const;

			static bool AllocateDestBuffer(Device* pDevice, Buffer* pDestBuffer, size_t byteWidth,
				uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
				uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
				uint32_t maxThreads = 1);

			static size_t Align(size_t byteSize);

		protected:
			bool prebuild(Device* pDevice);
			bool allocate(Device* pDevice, size_t byteWidth, DescriptorTableLib* pDescriptorTableLib,
				uint32_t numSRVs, MemoryFlag memoryFlags, const wchar_t* name, uint32_t maxThreads);
			bool createAccelerationStructure(VkDeviceSize size);
			void destroyAccelerationStructure();
			void registerPostbuildBuffer();

			BuildDesc_VK m_buildDesc;
			PrebuildInfo m_prebuildInfo;

			Buffer::sptr m_resource;
			Buffer::sptr m_postbuildInfoRB;
			Buffer::uptr m_postbuildInfo;

			XUSG::Device_VK* m_pVkDevice;
			uintptr_t m_byteOffset;
		};

		class BottomLevelAS_VK :
			public virtual BottomLevelAS,
			public virtual AccelerationStructure_VK
		{
		public:
			BottomLevelAS_VK();
			virtual ~BottomLevelAS_VK();

			bool Prebuild(Device* pDevice, uint32_t numGeometries, const GeometryBuffer& geometries,
				BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);
			bool Allocate(Device* pDevice, DescriptorTableLib* pDescriptorTableLib, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1);

			void Build(CommandList* pCommandList, const Resource* pScratch,
				const BottomLevelAS* pSource = nullptr, uint8_t numPostbuildInfoDescs = 0,
				const PostbuildInfoType* pPostbuildInfoTypes = nullptr);

			static void SetGeometries(GeometryBuffer& geometries, uint32_t numGeometries, GeometryDesc* pGeometries);

			static size_t AlignTransform(size_t byteSize);
			static size_t AlignAABB(size_t byteSize);
		};

		class TopLevelAS_VK :
			public TopLevelAS,
			public AccelerationStructure_VK
		{
		public:
			TopLevelAS_VK();
			virtual ~TopLevelAS_VK();

			bool Prebuild(Device* pDevice, uint32_t numInstances, BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);
			bool Allocate(Device* pDevice, DescriptorTableLib* pDescriptorTableLib, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1);

			void SetDestination(Device* pDevice, const Buffer::sptr destBuffer, uintptr_t byteOffset,
				uint32_t uavIndex, uint32_t srvIndex, DescriptorTableLib* pDescriptorTableLib);
			void Build(CommandList* pCommandList, const Resource* pScratch,
				const Resource* pInstanceDescs, const DescriptorHeap& descriptorHeap,
				const TopLevelAS* pSource = nullptr, uint8_t numPostbuildInfoDescs = 0,
				const PostbuildInfoType* pPostbuildInfoTypes = nullptr);

			const Descriptor& GetSRV() const;

			static void SetInstances(Device* pDevice, Buffer* pInstances, uint32_t numInstances,
				const InstanceDesc* pInstanceDescs, MemoryFlag memoryFlags = MemoryFlag::NONE,
				const wchar_t* instanceName = nullptr);

			static size_t AlignInstanceDesc(size_t byteSize);

		protected:
			uint32_t m_srvIndex;
		};

		class OpacityMicromapArray_VK :
			public OpacityMicromapArray,
			public AccelerationStructure_VK
		{
		public:
			OpacityMicromapArray_VK();
			virtual ~OpacityMicromapArray_VK();

			bool Prebuild(Device* pDevice, uint32_t numOpacityMicromaps, const GeometryBuffer& ommArrayDescs,
				BuildFlag flags = BuildFlag::PREFER_FAST_TRACE);
			bool Allocate(Device* pDevice, DescriptorTableLib* pDescriptorTableLib = nullptr, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1);

			void SetDestination(Device* pDevice, const Buffer::sptr destBuffer, uintptr_t byteOffset);
			void Build(CommandList* pCommandList, const Resource* pScratch,
				const OpacityMicromapArray* pSource = nullptr, uint8_t numPostbuildInfoDescs = 0,
				const PostbuildInfoType* pPostbuildInfoTypes = nullptr);

			static void SetOmmArray(GeometryBuffer& ommArrayDescs, uint32_t numOmmArrays, const Desc* pOmmArrayDescs);
			static void SetOmmDescs(Device* pDevice, Buffer* pOmmDescsBuffer, uint32_t numOmmDescs,
				const OpacityMicromapDesc* pOmmDescs, MemoryFlag memoryFlags = MemoryFlag::NONE,
				const wchar_t* ommName = nullptr);

			static size_t AlignOmmInput(size_t byteSize);
			static size_t AlignOmmDesc(size_t byteSize);

			VkMicromapEXT GetMicromap() const;

		protected:
			bool createMicromap(VkDeviceSize size);
			void destroyMicromap();

			BuildFlag m_buildFlags;
			VkMicromapEXT m_micromap;
		};
	}
}
