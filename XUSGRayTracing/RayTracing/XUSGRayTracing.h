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
