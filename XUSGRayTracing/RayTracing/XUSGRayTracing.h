//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Ultimate/XUSGUltimate.h"

namespace XUSG
{
	namespace RayTracing
	{
		enum class BuildFlag
		{
			NONE = 0,
			ALLOW_UPDATE = (1 << 0),
			ALLOW_COMPACTION = (1 << 1),
			PREFER_FAST_TRACE = (1 << 2),
			PREFER_FAST_BUILD = (1 << 3),
			MINIMIZE_MEMORY = (1 << 4),
			PERFORM_UPDATE = (1 << 5),
			ALLOW_OMM_UPDATE = (1 << 6),
			ALLOW_DISABLE_OMMS = (1 << 7)
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(BuildFlag);

		enum class GeometryFlag
		{
			NONE = 0,
			FULL_OPAQUE = (1 << 0),
			NO_DUPLICATE_ANYHIT_INVOCATION = (1 << 1)
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(GeometryFlag);

		enum class InstanceFlag : uint8_t
		{
			NONE = 0,
			TRIANGLE_CULL_DISABLE = (1 << 0),
			TRIANGLE_FRONT_COUNTERCLOCKWISE = (1 << 1),
			FORCE_OPAQUE = (1 << 2),
			FORCE_NON_OPAQUE = (1 << 3),
			FORCE_OMM_2_STATE = (1 << 4),
			DISABLE_OMMS = (1 << 5)
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(InstanceFlag);

		enum class PipelineFlag
		{
			NONE = 0,
			SKIP_TRIANGLES = (1 << 8),
			SKIP_PROCEDURAL_PRIMITIVES = (1 << 9),
			ALLOW_OPACITY_MICROMAPS = (1 << 10),
			ALLOW_CLUSTERED_GEOMETRY = (1 << 11)
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(PipelineFlag);

		enum class RTASOperationType
		{
			MOVE_CLUSTER_OBJECTS = 0,
			BUILD_BLAS_FROM_CLAS = 0x1,
			BUILD_CLAS_FROM_TRIANGLES = 0x2,
			BUILD_CLUSTER_TEMPLATES_FROM_TRIANGLES = 0x3,
			INSTANTIATE_CLUSTER_TEMPLATES = 0x4,
			PARTITIONED_TLAS = 0x8
		};

		enum class RTASOperationFlag
		{
			NONE = 0,
			FAST_TRACE = 0x4,
			FAST_OPERATION = 0x8,
			ALLOW_DATA_ACCESS = 0x100,
			ALLOW_OMM = 0x400
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(RTASOperationFlag);

		enum class RTASOperationMode
		{
			IMPLICIT_DESTINATIONS = 0,
			EXPLICIT_DESTINATIONS = 0x1,
			GET_SIZES = 0x2
		};

		enum class RTASMoveOperationType
		{
			CLUSTER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE = 0,
			CLUSTER_LEVEL_ACCELERATION_STRUCTURE = 0x1,
			CLUSTER_TEMPLATE = 0x2
		};

		enum class RTASClusterMoveFlag
		{
			NONE = 0,
			NO_OVERLAP = 0x200,
			ALLOW_OMM = 0x400
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(RTASClusterMoveFlag);

		enum class RTASPartitionedTLASFlag
		{
			NONE = 0,
			FAST_TRACE = 0x1,
			FAST_BUILD = 0x2,
			ENABLE_PARTITION_TRANSLATION = 0x4
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(RTASPartitionedTLASFlag);

		enum class RTASExecuteFlag
		{
			NONE = 0
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(RTASExecuteFlag);

		enum class ClusterVertexFormat
		{
			FLOAT32_2 = 1,
			FLOAT32_3 = 2,
			FLOAT16_2 = 3,
			FLOAT16_3 = 4,
			UNORM16_2 = 5,
			UNORM16_3 = 6,
			SNORM16_2 = 7,
			SNORM16_3 = 8,
			UNORM8_2 = 9,
			UNORM8_3 = 10,
			SNORM8_2 = 11,
			SNORM8_3 = 12,
			UNORM10_10_10_2 = 13,
			COMPRESSED1 = 14
		};

		enum class ClusterIndexFormat
		{
			NONE = 0,
			UINT8 = 0x1,
			UINT16 = 0x2,
			UINT32 = 0x4
		};

		enum class RTASClusterOperationClasFlag
		{
			NONE = 0,
			ALLOW_DISABLE_OMMS = 0x1,
			DISALLOW_DATA_ACCESS = 0x2
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(RTASClusterOperationClasFlag);

		enum class RTASClusteredGeometryFlag : uint32_t
		{
			NONE = 0,
			FULL_OPAQUE = 0x80000000,
			NO_DUPLICATE_ANYHIT_INVOCATION = 0x40000000
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(RTASClusteredGeometryFlag);

		enum class RTASAddressResolutionFlag
		{
			NONE = 0,
			INDIRECTED_BATCH_RESULT = 0x1,
			INDIRECTED_BATCH_SCRATCH = 0x2,
			INDIRECTED_DESTINATION_ADDRESS_ARRAY = 0x4,
			INDIRECTED_RESULT_SIZE_ARRAY = 0x8,
			INDIRECTED_INDIRECT_ARGUMENT_ARRAY = 0x10,
			INDIRECTED_INDIRECT_ARGUMENT_COUNT = 0x20
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(RTASAddressResolutionFlag);

		enum class RTASPartitionedTLASOperationType
		{
			WRITE_INSTANCE = 0,
			UPDATE_INSTANCE = 0x1,
			TRANSLATE_PARTITION = 0x2
		};

		enum class RTASPartitionedTLASPartitionIndex : uint32_t
		{
			GLOBAL_PARTITION = 0xffffffff
		};

		enum class RTASPartitionedTLASInstanceFlag
		{
			NONE = 0,
			TRIANGLE_CULL_DISABLE = 0x1,
			TRIANGLE_FRONT_COUNTERCLOCKWISE = 0x2,
			FORCE_OPAQUE = 0x4,
			FORCE_NON_OPAQUE = 0x8,
			FORCE_OMM_2_STATE = 0x10,
			DISABLE_OMMS = 0x20,
			ENABLE_EXPLICIT_AABB = 0x40
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(RTASPartitionedTLASInstanceFlag);

		enum class GeometryType : uint8_t
		{
			TRIANGLES,
			PROCEDURAL_PRIMITIVE_AABBS,
			OMM_TRIANGLES
		};

		enum class HitGroupType : uint8_t
		{
			TRIANGLES,
			PROCEDURAL_PRIMITIVE
		};

		enum class PostbuildInfoType : uint8_t
		{
			COMPACTED_SIZE,
			TOOLS_VISUALIZATION,
			SERIALIZATION,
			CURRENT_SIZE
		};

		enum class CopyMode : uint8_t
		{
			CLONE,
			COMPACT,
			VISUALIZATION_DECODE_FOR_TOOLS,
			SERIALIZE,
			DESERIALIZE
		};

		enum class OMMSpecialIndex
		{
			FULLY_TRANSPARENT = -1,
			FULLY_OPAQUE = -2,
			FULLY_UNKNOWN_TRANSPARENT = -3,
			FULLY_UNKNOWN_OPAQUE = -4
		};

		enum class OpacityMicromapFormat : uint16_t
		{
			OC1_2_STATE = 0x1,
			OC1_4_STATE = 0x2
		};

		using BuildDesc = void;
		using GeometryBuffer = std::vector<uint8_t>;

		struct PrebuildInfo
		{
			uint64_t ResultDataMaxByteSize;
			uint64_t ScratchDataByteSize;
			uint64_t UpdateScratchDataByteSize;
		};

		struct PostbuildInfo
		{
			uint64_t DestBuffer;
			PostbuildInfoType InfoType;
		};

		struct ResourceView
		{
			const Resource* pResource;
			uintptr_t Offset;
		};

		// 8 bytes. Matches D3D12_RAYTRACING_OPACITY_MICROMAP_DESC and VkMicromapTriangleKHR.
		// Format values are the D3D12 enum. Vulkan converts them in SetOmmDescs.
		struct OpacityMicromapDesc
		{
			uint32_t ByteOffset;
			uint16_t SubdivisionLevel;
			OpacityMicromapFormat Format;
		};

		struct OMMHistogramEntry
		{
			uint32_t Count;
			uint32_t SubdivisionLevel;
			OpacityMicromapFormat Format;
		};

		// Matches D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE.
		struct GpuVirtualAddressAndStride
		{
			uint64_t StartAddress;
			uint64_t StrideInBytes;
		};

		// Matches D3D12_RTAS_OPERATION_PREBUILD_INFO.
		struct RTASPrebuildInfo
		{
			uint64_t ResultDataMaxByteSize;
			uint64_t ScratchDataByteSize;
		};

		struct RTASClusterLimits
		{
			uint32_t MaxArgCount;
			uint32_t MaxGeometryIndexValue;
			uint32_t MaxUniqueGeometryIndexAndFlagsCountPerCluster;
			uint32_t MaxTriangleCountPerCluster;
			uint32_t MaxUniqueVertexIndexCountPerCluster;
			uint32_t MaxTotalTriangleCount;
			uint32_t MaxSumOfUniqueVertexIndexCounts;
			uint32_t MaxOpacityMicromapIndicesPerCluster;
		};

		struct RTASClusterTrianglesInputs
		{
			RTASClusterLimits ClusterLimits;
			RTASOperationFlag Flags;
			RTASOperationMode Mode;
			ClusterVertexFormat VertexFormat;
			ClusterIndexFormat IndexFormat;
			ClusterIndexFormat GeometryIndexAndFlagsIndexFormat;
			ClusterIndexFormat OpacityMicromapIndexFormat;
			union
			{
				uint32_t MinPositionTruncateBitCount;
				uint32_t MaxCompressedClusterPositionsSize;
			};
		};

		struct RTASClusterTemplateTrianglesInputs
		{
			RTASClusterLimits ClusterLimits;
			RTASOperationFlag Flags;
			RTASOperationMode Mode;
			ClusterVertexFormat VertexHintFormat;
			ClusterVertexFormat VertexInstantiationFormat;
			ClusterIndexFormat IndexFormat;
			ClusterIndexFormat GeometryIndexAndFlagsIndexFormat;
			ClusterIndexFormat OpacityMicromapIndexFormat;
		};

		struct RTASInstantiateClusterTemplateInputs
		{
			RTASClusterLimits ClusterLimits;
			RTASOperationFlag Flags;
			RTASOperationMode Mode;
			ClusterVertexFormat VertexSourceFormat;
		};

		struct RTASClasInputs
		{
			RTASOperationFlag Flags;
			uint32_t MaxArgCount;
			RTASOperationMode Mode;
			uint32_t MaxTotalClasCount;
			uint32_t MaxClasCountPerArg;
		};

		struct RTASClusterMoves
		{
			RTASClusterMoveFlag Flags;
			uint32_t MaxArgCount;
			RTASOperationMode Mode;
			RTASMoveOperationType Type;
			uint64_t MaxBytesMoved;
		};

		struct RTASBatchedOperationData
		{
			RTASAddressResolutionFlag AddressResolutionFlags;
			uint64_t BatchResultData;
			uint64_t BatchScratchData;
			GpuVirtualAddressAndStride ResultAddressArray;
			GpuVirtualAddressAndStride ResultSizeArray;
			GpuVirtualAddressAndStride IndirectArgumentArray;
			uint64_t IndirectArgumentArraySize;
			uint64_t ToolsInfo;
		};

		struct RTASPartitionedTLASInputs
		{
			RTASPartitionedTLASFlag Flags;
			uint32_t InstanceCount;
			uint32_t MaxInstancePerPartitionCount;
			uint32_t PartitionCount;
			uint32_t MaxInstanceInGlobalPartitionCount;
		};

		struct RTASPartitionedTLASOperationData
		{
			uint64_t SourceAccelerationStructureData;
			uint64_t DestAccelerationStructureData;
			uint64_t ScratchAccelerationStructureData;
			uint64_t IndirectPartitionedTlasOpCount;
			uint64_t IndirectPartitionedTlasOps;
		};

		struct RTASOperationInputs
		{
			RTASOperationType Type;
			union
			{
				const RTASClasInputs* pClasDesc;
				const RTASClusterTrianglesInputs* pClusterTrianglesDesc;
				const RTASClusterTemplateTrianglesInputs* pClusterTemplateTrianglesDesc;
				const RTASInstantiateClusterTemplateInputs* pInstantiateClusterTemplateDesc;
				const RTASClusterMoves* pClusterMovesDesc;
				const RTASPartitionedTLASInputs* pPartitionedTLASInputsDesc;
			};
		};

		struct RTASOperationDesc
		{
			RTASOperationInputs Inputs;
			union
			{
				const RTASBatchedOperationData* pBatchedOperationData;
				const RTASPartitionedTLASOperationData* pPartitionedTlasOperationData;
			};
		};

		// GPU argument layouts match the D3D12_RTAS_OPERATION_*_ARGS structs.
		struct RTASBuildClasFromTrianglesArgs
		{
			uint32_t ClusterID;
			uint32_t ClusterFlags;
			uint16_t TriangleCount;
			uint16_t UniqueVertexIndexCount;
			uint32_t BaseGeometryIndexAndFlags;
			uint32_t OpacityMicromapBaseLocation;
			uint16_t VertexBufferStride;
			uint16_t IndexBufferStride;
			uint16_t OpacityMicromapIndexBufferStride;
			uint16_t GeometryIndexAndFlagsArrayStride;
			uint16_t PositionTruncateBitCount;
			uint16_t ReservedPadding;
			uint64_t VertexBuffer;
			uint64_t IndexBuffer;
			uint64_t GeometryIndexAndFlagsArray;
			uint64_t GeometryIndexAndFlagsIndexBuffer;
			uint64_t OpacityMicromapArray;
			uint64_t OpacityMicromapIndexBuffer;
		};

		struct RTASBuildBlasFromClasArgs
		{
			uint32_t ClasAddressCount;
			uint32_t ClasAddressStride;
			uint64_t ClasAddressArray;
		};

		struct RTASMoveClusterObjectsArgs
		{
			uint64_t SourceAccelerationStructure;
		};

		struct RTASBuildClusterTemplatesFromTrianglesArgs
		{
			RTASBuildClasFromTrianglesArgs TrianglesArgs;
			uint64_t InstantiationBoundingBoxLimit;
			union
			{
				struct
				{
					uint32_t field0;
					uint32_t ReservedPadding;
				} Compressed1;
			};
		};

		struct RTASInstantiateClusterTemplatesArgs
		{
			int32_t GeometryIndexOffset;
			uint32_t ClusterIdOffset;
			uint64_t ClusterTemplate;
			GpuVirtualAddressAndStride VertexBuffer;
		};

		struct RTASPartitionedTLASOperation
		{
			RTASPartitionedTLASOperationType Type;
			uint32_t ArgCount;
			GpuVirtualAddressAndStride ArgData;
		};

		struct RTASPartitionedTLASWriteInstanceArgs
		{
			float Transform[3][4];
			uint32_t InstanceID;
			uint32_t InstanceMask;
			uint32_t InstanceContributionToHitGroupIndex;
			uint32_t InstanceFlags;
			uint64_t AccelerationStructure;
			uint32_t InstanceIndex;
			uint32_t PartitionIndex;
			float MinX;
			float MinY;
			float MinZ;
			float MaxX;
			float MaxY;
			float MaxZ;
		};

		struct RTASPartitionedTLASUpdateInstanceArgs
		{
			uint32_t InstanceIndex;
			uint32_t InstanceContributionToHitGroupIndex;
			uint64_t AccelerationStructure;
		};

		struct RTASPartitionedTLASTranslatePartitionArgs
		{
			uint32_t PartitionIndex;
			float PartitionTranslation[3];
		};

		//--------------------------------------------------------------------------------------
		// Device
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE Device :
			public virtual XUSG::Device
		{
		public:
			//Device();
			virtual ~Device() {};

			// Same as XUSG::Device::Create, then initializes the ray-tracing interface.
			// flags is for the DX12 fallback layer; Vulkan ignores it.
			virtual uint32_t Create(void* pAdapter, uint32_t minFeatureLevel, const wchar_t* name = nullptr) = 0;
			virtual uint32_t Create(void* pAdapter, uint32_t minFeatureLevel, uint8_t flags,
				const wchar_t* name = nullptr) = 0;
			virtual void Create(void* pNativeHandle) = 0;

			virtual void* GetRTHandle() const = 0;

			// Concrete RT backend (Device_DX12* / Device_VK*). Distinct from
			// XUSG::Device::GetBackend(), which returns the core Device_* subobject.
			virtual void* GetRTBackend() = 0;

			virtual bool HasClustersAndPTLAS() const = 0;
			virtual void GetRTASOperationPrebuildInfo(const RTASOperationInputs& desc, RTASPrebuildInfo& info) const = 0;

			using uptr = std::unique_ptr<Device>;
			using sptr = std::shared_ptr<Device>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		//--------------------------------------------------------------------------------------
		// Acceleration structure
		//--------------------------------------------------------------------------------------
		class CommandList;

		class XUSG_INTERFACE AccelerationStructure
		{
		public:
			//AccelerationStructure();
			virtual ~AccelerationStructure() {}

			// Auto allocate a buffer with byteWidth = GetResultDataMaxByteSize() when setting byteWidth = 0
			virtual bool Allocate(Device* pDevice, DescriptorTableLib* pDescriptorTableLib, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1) = 0;

			virtual void SetDestination(Device* pDevice, const Buffer::sptr destBuffer,
				uintptr_t byteOffset, uint32_t uavIndex, DescriptorTableLib* pDescriptorTableLib) = 0;

			virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers = 0) = 0;

			virtual const PrebuildInfo& GetPrebuildInfo() const = 0;

			virtual Buffer* GetPostbuildInfo() const = 0;

			virtual size_t GetResultDataMaxByteSize(bool isAligned = true) const = 0;
			virtual size_t GetScratchDataByteSize(bool isAligned = true) const = 0;
			virtual size_t GetUpdateScratchDataByteSize(bool isAligned = true) const = 0;
			virtual size_t GetCompactedByteSize(bool isAligned = true) const = 0;

			virtual uint64_t GetVirtualAddress() const = 0;
			virtual uint64_t GetResourcePointer() const = 0;

			// Backend C++ object (AccelerationStructure_DX12* / AccelerationStructure_VK*, not available to users), internal use only.
			virtual void* GetBackend() = 0;

			static bool AllocateDestBuffer(Device* pDevice, Buffer* pDestBuffer, size_t byteWidth,
				uint32_t numSRVs = 1, const uintptr_t* firstSrvElements = nullptr,
				uint32_t numUAVs = 1, const uintptr_t* firstUavElements = nullptr,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr,
				uint32_t maxThreads = 1);
			static bool AllocateUAVBuffer(Device* pDevice, Buffer* pBuffer, size_t byteWidth,
				ResourceState dstState = ResourceState::COMMON, MemoryFlag memoryFlags = MemoryFlag::NONE,
				const wchar_t* name = nullptr,
				uint32_t maxThreads = 1);
			static bool AllocateUploadBuffer(Device* pDevice, Buffer* pBuffer, size_t byteWidth,
				void* pData, MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr);

			static uint32_t SetBarrier(ResourceBarrier* pBarriers, Resource* pResource, uint32_t numBarriers = 0);

			static size_t Align(size_t byteSize);
		};

		//--------------------------------------------------------------------------------------
		// Bottom-level acceleration structure
		//--------------------------------------------------------------------------------------
		class OpacityMicromapArray;

		class XUSG_INTERFACE BottomLevelAS :
			public virtual AccelerationStructure
		{
		public:
			struct TriangleGeometry
			{
				uint64_t Transform;
				Format IndexFormat;
				Format VertexFormat;
				uint32_t IndexCount;
				uint32_t VertexCount;
				uint64_t IndexBuffer;
				uint64_t VertexBufferStart;
				uint16_t VertexBufferStride;
			};

			struct AABBGeometry
			{
				uint32_t AABBCount;
				uint64_t AABBsStart;
				uint16_t AABBsStride;
			};

			struct OMMLinkage
			{
				uint64_t OpacityMicromapIndexBufferStart;
				uint16_t OpacityMicromapIndexBufferStride;
				Format OpacityMicromapIndexFormat;
				uint32_t OpacityMicromapBaseLocation;
				OpacityMicromapArray* pOpacityMicromapArray;
				uintptr_t OpacityMicromapArrayOffset;
			};

			struct OMMTriangleGeometry
			{
				const TriangleGeometry* pTriangles;
				const OMMLinkage* pOmmLinkage;
			};

			struct GeometryDesc
			{
				GeometryType Type;
				GeometryFlag Flags;
				union
				{
					TriangleGeometry Triangles;
					AABBGeometry AABBs;
					OMMTriangleGeometry OmmTriangles;
				};
			};

			//BottomLevelAS();
			virtual ~BottomLevelAS() {}

			virtual bool Prebuild(Device* pDevice, uint32_t numGeometries, const GeometryBuffer& geometries,
				BuildFlag flags = BuildFlag::PREFER_FAST_TRACE) = 0;

			virtual void Build(CommandList* pCommandList, const Resource* pScratch,
				BottomLevelAS* pSource = nullptr, uint8_t numPostbuildInfoDescs = 0,
				const PostbuildInfoType* pPostbuildInfoTypes = nullptr) = 0;

			static void SetTriangleGeometry(TriangleGeometry& triangles, Format vertexFormat,
				const VertexBufferView& vbv, const IndexBufferView* pIbv = nullptr,
				GeometryFlag flags = GeometryFlag::FULL_OPAQUE, const ResourceView* pTransform = nullptr);
			static void SetTriangleGeometry(GeometryDesc& geometry, Format vertexFormat,
				const VertexBufferView& vbv, const IndexBufferView* pIbv = nullptr,
				GeometryFlag flags = GeometryFlag::FULL_OPAQUE, const ResourceView* pTransform = nullptr);
			static void SetAABBGeometry(GeometryDesc& geometry, const VertexBufferView& vbv,
				GeometryFlag flags = GeometryFlag::NONE);
			static void SetOMMGeometry(GeometryDesc& geometry, const TriangleGeometry* pTriangles,
				const OMMLinkage* pOmmLinkage, GeometryFlag flags = GeometryFlag::NONE);
			static void SetGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
				GeometryDesc* pGeometries);

			static size_t AlignTransform(size_t byteSize);
			static size_t AlignAABB(size_t byteSize);

			using uptr = std::unique_ptr<BottomLevelAS>;
			using sptr = std::shared_ptr<BottomLevelAS>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		//--------------------------------------------------------------------------------------
		// Top-level acceleration structure
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE TopLevelAS :
			public virtual AccelerationStructure
		{
		public:
			struct InstanceDesc
			{
				const float* pTransform;
				unsigned int InstanceID : 24;
				unsigned int InstanceMask : 8;
				unsigned int InstanceContributionToHitGroupIndex : 24;
				unsigned int Flags : 8;
				const BottomLevelAS* pBottomLevelAS;
			};

			//TopLevelAS();
			virtual ~TopLevelAS() {}

			virtual bool Prebuild(Device* pDevice, uint32_t numInstances,
				BuildFlag flags = BuildFlag::PREFER_FAST_TRACE) = 0;

			virtual void SetDestination(Device* pDevice, const Buffer::sptr destBuffer, uintptr_t byteOffset,
				uint32_t uavIndex, uint32_t srvIndex, DescriptorTableLib* pDescriptorTableLib) = 0;
			virtual void Build(CommandList* pCommandList, const Resource* pScratch,
				const Resource* pInstanceDescs, const DescriptorHeap& descriptorHeap,
				TopLevelAS* pSource = nullptr, uint8_t numPostbuildInfoDescs = 0,
				const PostbuildInfoType* pPostbuildInfoTypes = nullptr) = 0;

			virtual const Descriptor& GetSRV() const = 0;

			static void SetInstance(InstanceDesc& instanceDesc, const BottomLevelAS* pBottomLevelAS,
				const float* transform, InstanceFlag flags = InstanceFlag::NONE, uint32_t instanceID = 0,
				uint8_t instanceMask = 1, uint32_t instanceContributionToHitGroupIndex = 0);
			static void SetInstances(Device* pDevice, Buffer* pInstances, uint32_t numInstances,
				const InstanceDesc* pInstanceDescs, MemoryFlag memoryFlags = MemoryFlag::NONE,
				const wchar_t* instanceName = nullptr);

			static size_t AlignInstanceDesc(size_t byteSize);

			using uptr = std::unique_ptr<TopLevelAS>;
			using sptr = std::shared_ptr<TopLevelAS>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		//--------------------------------------------------------------------------------------
		// Opacity micromap array
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE OpacityMicromapArray :
			public virtual AccelerationStructure
		{
		public:
			struct Desc
			{
				uint32_t NumOmmHistogramEntries;
				const OMMHistogramEntry* pOmmHistogram;
				Buffer* pInputBuffer;
				uintptr_t InputOffset;
				Buffer* pPerOmmDescs;
				uintptr_t PerOmmDescOffset;
				size_t PerOmmDescStride;
			};

			//OpacityMicromapArray();
			virtual ~OpacityMicromapArray() {}

			virtual bool Prebuild(Device* pDevice, uint32_t numOpacityMicromaps, const GeometryBuffer& ommArrayDescs,
				BuildFlag flags = BuildFlag::PREFER_FAST_TRACE) = 0;

			virtual void SetDestination(Device* pDevice, const Buffer::sptr destBuffer, uintptr_t byteOffset) = 0;
			virtual void Build(CommandList* pCommandList, const Resource* pScratch,
				OpacityMicromapArray* pSource = nullptr, uint8_t numPostbuildInfoDescs = 0,
				const PostbuildInfoType* pPostbuildInfoTypes = nullptr) = 0;

			static void SetOmmArray(GeometryBuffer& ommArrayDescs, uint32_t numOpacityMicromaps,
				const Desc* pOmmArrayDescs);
			static void SetOmmDescs(Device* pDevice, Buffer* pOmmDescsBuffer, uint32_t numOmmDescs,
				const OpacityMicromapDesc* pOmmDescs, MemoryFlag memoryFlags = MemoryFlag::NONE,
				const wchar_t* ommName = nullptr);

			static size_t AlignOmmInput(size_t byteSize);
			static size_t AlignOmmDesc(size_t byteSize);

			using uptr = std::unique_ptr<OpacityMicromapArray>;
			using sptr = std::shared_ptr<OpacityMicromapArray>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		//--------------------------------------------------------------------------------------
		// Cluster-level acceleration structure
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE ClusterLevelAS
		{
		public:
			//ClusterLevelAS();
			virtual ~ClusterLevelAS() {}

			virtual void SetTriangles(const RTASClusterTrianglesInputs& desc) = 0;
			virtual void SetTemplateInstantiation(const RTASInstantiateClusterTemplateInputs& desc) = 0;

			virtual bool Prebuild(Device* pDevice) = 0;
			virtual bool Allocate(Device* pDevice, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1) = 0;
			virtual void Build(CommandList* pCommandList, const Resource* pScratch,
				uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride = 0) = 0;

			virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers = 0) = 0;

			virtual const RTASPrebuildInfo& GetPrebuildInfo() const = 0;

			virtual size_t GetResultDataMaxByteSize(bool isAligned = true) const = 0;
			virtual size_t GetScratchDataByteSize(bool isAligned = true) const = 0;

			virtual uint64_t GetVirtualAddress() const = 0;

			virtual void* GetBackend() = 0;

			static size_t Align(size_t byteSize);

			using uptr = std::unique_ptr<ClusterLevelAS>;
			using sptr = std::shared_ptr<ClusterLevelAS>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		//--------------------------------------------------------------------------------------
		// Cluster template
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE ClusterTemplate
		{
		public:
			//ClusterTemplate();
			virtual ~ClusterTemplate() {}

			virtual void SetTriangles(const RTASClusterTemplateTrianglesInputs& desc) = 0;

			virtual bool Prebuild(Device* pDevice) = 0;
			virtual bool Allocate(Device* pDevice, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1) = 0;
			virtual void Build(CommandList* pCommandList, const Resource* pScratch,
				uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride = 0) = 0;
			virtual void Instantiate(CommandList* pCommandList, ClusterLevelAS* pDestination, const Resource* pScratch,
				uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride = 0) = 0;

			virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers = 0) = 0;

			virtual const RTASPrebuildInfo& GetPrebuildInfo() const = 0;

			virtual size_t GetResultDataMaxByteSize(bool isAligned = true) const = 0;
			virtual size_t GetScratchDataByteSize(bool isAligned = true) const = 0;

			virtual uint64_t GetVirtualAddress() const = 0;

			virtual void* GetBackend() = 0;

			static size_t Align(size_t byteSize);

			using uptr = std::unique_ptr<ClusterTemplate>;
			using sptr = std::shared_ptr<ClusterTemplate>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		//--------------------------------------------------------------------------------------
		// Cluster bottom-level acceleration structure
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE ClusterBottomLevelAS
		{
		public:
			//ClusterBottomLevelAS();
			virtual ~ClusterBottomLevelAS() {}

			virtual void SetClusters(const RTASClasInputs& desc) = 0;

			virtual bool Prebuild(Device* pDevice) = 0;
			virtual bool Allocate(Device* pDevice, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1) = 0;
			virtual void Build(CommandList* pCommandList, const Resource* pScratch,
				uint64_t indirectArgumentAddress, uint64_t indirectArgumentStride = 0) = 0;

			virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers = 0) = 0;

			virtual const RTASPrebuildInfo& GetPrebuildInfo() const = 0;

			virtual size_t GetResultDataMaxByteSize(bool isAligned = true) const = 0;
			virtual size_t GetScratchDataByteSize(bool isAligned = true) const = 0;

			virtual uint64_t GetVirtualAddress() const = 0;

			virtual void* GetBackend() = 0;

			static size_t Align(size_t byteSize);

			using uptr = std::unique_ptr<ClusterBottomLevelAS>;
			using sptr = std::shared_ptr<ClusterBottomLevelAS>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		//--------------------------------------------------------------------------------------
		// Partitioned top-level acceleration structure
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE PartitionedTLAS
		{
		public:
			//PartitionedTLAS();
			virtual ~PartitionedTLAS() {}

			virtual void SetPartitions(const RTASPartitionedTLASInputs& desc) = 0;

			virtual bool Prebuild(Device* pDevice) = 0;
			virtual bool Allocate(Device* pDevice, size_t byteWidth = 0,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr, uint32_t maxThreads = 1) = 0;
			virtual void Build(CommandList* pCommandList, const Resource* pScratch,
				uint64_t indirectOps, uint64_t indirectOpCount) = 0;
			virtual void Update(CommandList* pCommandList, const Resource* pScratch,
				uint64_t indirectOps, uint64_t indirectOpCount) = 0;

			virtual uint32_t SetBarrier(ResourceBarrier* pBarriers, uint32_t numBarriers = 0) = 0;

			virtual const RTASPrebuildInfo& GetPrebuildInfo() const = 0;

			virtual size_t GetResultDataMaxByteSize(bool isAligned = true) const = 0;
			virtual size_t GetScratchDataByteSize(bool isAligned = true) const = 0;

			virtual uint64_t GetVirtualAddress() const = 0;
			virtual const Descriptor& GetSRV() const = 0;

			virtual void* GetBackend() = 0;

			static size_t Align(size_t byteSize);

			using uptr = std::unique_ptr<PartitionedTLAS>;
			using sptr = std::shared_ptr<PartitionedTLAS>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		//--------------------------------------------------------------------------------------
		// Shader record
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE ShaderRecord
		{
		public:
			//ShaderRecord(const void* pShaderIdentifier, uint32_t shaderIdentifierSize,
				//const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0);
			//ShaderRecord(Device* pDevice, const Pipeline& pipeline, const wchar_t* shaderName,
				//const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0);
			virtual ~ShaderRecord() {}

			virtual void CopyTo(void* dest) const = 0;

			static const void* GetShaderIdentifier(const Pipeline& pipeline, const wchar_t* shaderName);

			static uint32_t GetShaderIdentifierSize(Device* pDevice);

			using uptr = std::unique_ptr<ShaderRecord>;
			using sptr = std::shared_ptr<ShaderRecord>;

			static uptr MakeUnique(void* pShaderIdentifier, uint32_t shaderIdentifierSize, const void* pLocalDescriptorArgs = nullptr,
				uint32_t localDescriptorArgSize = 0);
			static sptr MakeShared(void* pShaderIdentifier, uint32_t shaderIdentifierSize, const void* pLocalDescriptorArgs = nullptr,
				uint32_t localDescriptorArgSize = 0);
			static uptr MakeUnique(Device* pDevice, const Pipeline& pipeline, const wchar_t* shaderName,
				const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0); // shader - shader name for DX12
			static sptr MakeShared(Device* pDevice, const Pipeline& pipeline, const wchar_t* shaderName,
				const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0); // shader - shader name for DX12

			static size_t Align(uint32_t byteSize);
		};

		//--------------------------------------------------------------------------------------
		// Shader table
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE ShaderTable
		{
		public:
			//ShaderTable();
			virtual ~ShaderTable() {}

			virtual bool Create(XUSG::Device* pDevice, uint32_t numShaderRecords, uint32_t shaderRecordSize,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr) = 0;

			virtual void Create(Buffer::sptr resource, uint32_t shaderRecordSize, uintptr_t byteOffset) = 0;
			virtual void AddShaderRecord(const ShaderRecord* pShaderRecord) = 0;
			virtual void Reset() = 0;

			virtual uint64_t GetVirtualAddress() const = 0;
			virtual size_t GetByteSize() const = 0;
			virtual size_t GetByteStride() const = 0;

			using uptr = std::unique_ptr<ShaderTable>;
			using sptr = std::shared_ptr<ShaderTable>;

			static uptr MakeUnique();
			static sptr MakeShared();

			static size_t Align(size_t byteSize);
		};

		//--------------------------------------------------------------------------------------
		// Command list
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE CommandList :
			public virtual Ultimate::CommandList
		{
		public:
			//CommandList();
			virtual ~CommandList() {}

			virtual bool Create(Device* pDevice, uint32_t nodeMask, CommandListType type, uint8_t numAllocators,
				CommandAllocator* const* ppAllocators, const Pipeline& pipeline, const wchar_t* name = nullptr) = 0;

			virtual void BuildRaytracingAccelerationStructure(const BuildDesc* pDesc,
				uint32_t numPostbuildInfoDescs, const PostbuildInfo* pPostbuildInfoDescs,
				const DescriptorHeap* pDescriptorHeap = nullptr) = 0;
			virtual void EmitRaytracingAccelerationStructurePostbuildInfo(const PostbuildInfo* pDesc,
				uint32_t numAccelerationStructures, const uint64_t* pAccelerationStructureData) = 0;
			virtual void CopyRaytracingAccelerationStructure(AccelerationStructure* pDst,
				AccelerationStructure* pSrc, CopyMode mode,
				const DescriptorHeap* pDescriptorHeap = nullptr) = 0;
			virtual void ExecuteIndirectRTASOperations(uint32_t numOperationDescs, const RTASOperationDesc* pDescs,
				RTASExecuteFlag flags = RTASExecuteFlag::NONE) = 0;

			virtual void SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps) = 0;
			virtual void SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const = 0;
			virtual void SetTopLevelAccelerationStructure(uint32_t index, uint64_t topLevelASPtr) const = 0;
			virtual void SetRayTracingPipeline(const Pipeline& pipeline) const = 0;
			virtual void DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
				const ShaderTable* pRayGen, const ShaderTable* pHitGroup, const ShaderTable* pMiss,
				const ShaderTable* pCallable = nullptr) const = 0;

			virtual Device* GetRTDevice() const = 0;

			// Concrete RT backend (CommandList_DX12* / CommandList_VK*). Distinct from
			// XUSG::CommandList::GetBackend(), which returns the core CommandList_* subobject.
			virtual void* GetRTBackend() = 0;

			using uptr = std::unique_ptr<CommandList>;
			using sptr = std::shared_ptr<CommandList>;

			static uptr MakeUnique();
			static sptr MakeShared();
			static uptr MakeUnique(XUSG::CommandList* pCommandList, RayTracing::Device* pDevice);
			static sptr MakeShared(XUSG::CommandList* pCommandList, RayTracing::Device* pDevice);
		};

		//--------------------------------------------------------------------------------------
		// Pipeline layout
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE PipelineLayout :
			public virtual Util::PipelineLayout
		{
		public:
			//PipelineLayout();
			virtual ~PipelineLayout() {}

			virtual XUSG::PipelineLayout CreatePipelineLayout(Device* pDevice, PipelineLayoutLib* pPipelineLayoutLib,
				PipelineLayoutFlag flags, const wchar_t* name = nullptr) = 0;
			virtual XUSG::PipelineLayout GetPipelineLayout(Device* pDevice, PipelineLayoutLib* pPipelineLayoutLib,
				PipelineLayoutFlag flags, const wchar_t* name = nullptr) = 0;

			using uptr = std::unique_ptr<PipelineLayout>;
			using sptr = std::shared_ptr<PipelineLayout>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		//--------------------------------------------------------------------------------------
		// Pipeline state
		//--------------------------------------------------------------------------------------
		class PipelineLib;

		class XUSG_INTERFACE State
		{
		public:
			//State();
			virtual ~State() {}

			virtual void SetShaderLibrary(uint32_t index, const Blob& shaderLib,
				uint32_t numShaders = 0, const wchar_t** pShaderNames = nullptr) = 0;
			virtual void SetHitGroup(uint32_t index, const wchar_t* hitGroupName, const wchar_t* closestHitShaderName,
				const wchar_t* anyHitShaderName = nullptr, const wchar_t* intersectionShaderName = nullptr,
				HitGroupType type = HitGroupType::TRIANGLES) = 0;
			virtual void SetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize) = 0;
			virtual void SetLocalPipelineLayout(uint32_t index, const XUSG::PipelineLayout& layout,
				uint32_t numShaders, const wchar_t** pShaderNames) = 0;
			virtual void SetGlobalPipelineLayout(const XUSG::PipelineLayout& layout) = 0;
			virtual void SetMaxRecursionDepth(uint32_t depth) = 0;
			virtual void SetNodeMask(uint32_t nodeMask) = 0;
			virtual void SetPipelineFlags(PipelineFlag flags) = 0;

			virtual Pipeline CreatePipeline(PipelineLib* pPipelineCache, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(PipelineLib* pPipelineCache, const wchar_t* name = nullptr) = 0;

			virtual const wchar_t* GetHitGroupName(uint32_t index) = 0;
			virtual uint32_t GetNumHitGroups() = 0;

			// Backend C++ object (RayTracing::State_DX12* / State_VK*, not available to users), internal use only.
			virtual void* GetBackend() = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique();
			static sptr MakeShared();
		};

		class XUSG_INTERFACE PipelineLib
		{
		public:
			//PipelineLib();
			//PipelineLib(Device* pDevice);
			virtual ~PipelineLib() {}

			virtual void SetDevice(Device* pDevice) = 0;
			virtual void SetPipeline(State* pState, const Pipeline& pipeline) = 0;

			virtual Pipeline CreatePipeline(State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(State* pState, const wchar_t* name = nullptr) = 0;

			using uptr = std::unique_ptr<PipelineLib>;
			using sptr = std::shared_ptr<PipelineLib>;

			static uptr MakeUnique();
			static sptr MakeShared();
			static uptr MakeUnique(Device* pDevice);
			static sptr MakeShared(Device* pDevice);
		};
	}
}
