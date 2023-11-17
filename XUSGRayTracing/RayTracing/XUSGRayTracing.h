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
			PERFORM_UPDATE = (1 << 5)
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(BuildFlag);

		enum class GeometryFlag
		{
			NONE = 0,
			FULL_OPAQUE = (1 << 0),
			NO_DUPLICATE_ANYHIT_INVOCATION = (1 << 1)
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(GeometryFlag);

		enum class InstanceFlag {
			NONE = 0,
			TRIANGLE_CULL_DISABLE = (1 << 0),
			TRIANGLE_FRONT_COUNTERCLOCKWISE = (1 << 1),
			FORCE_OPAQUE = (1 << 2),
			FORCE_NON_OPAQUE = (1 << 3)
		};

		XUSG_DEF_ENUM_FLAG_OPERATORS(InstanceFlag);

		enum class HitGroupType : uint8_t
		{
			TRIANGLES,
			PROCEDURAL
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

		using BuildDesc = void;
		using GeometryBuffer = std::vector<uint8_t>;

		struct PrebuildInfo
		{
			uint64_t ResultDataMaxSizeInBytes;
			uint64_t ScratchDataSizeInBytes;
			uint64_t UpdateScratchDataSizeInBytes;
		};

		struct PostbuildInfo
		{
			uint64_t DestBuffer;
			PostbuildInfoType InfoType;
		};

		struct ResourceView
		{
			const Resource* pResource;
			uint64_t Offset;
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

			virtual bool CreateInterface(uint8_t flags = 0) = 0;

			virtual void* GetRTHandle() const = 0;

			using uptr = std::unique_ptr<Device>;
			using sptr = std::shared_ptr<Device>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
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

			// Auto allocate a buffer with byteWidth = GetResultDataMaxSize() when setting byteWidth = 0
			virtual bool Allocate(const Device* pDevice, uint32_t descriptorIndex, size_t byteWidth = 0) = 0;

			virtual Buffer::sptr GetResource() const = 0;
			virtual Buffer::sptr GetPostbuildInfo() const = 0;

			virtual uint32_t GetResultDataMaxSize() const = 0;
			virtual uint32_t GetScratchDataMaxSize() const = 0;
			virtual uint32_t GetUpdateScratchDataSize() const = 0;

			virtual uint64_t GetResourcePointer() const = 0;

			static uint32_t GetUAVCount();

			static void SetUAVCount(uint32_t numUAVs);

			static bool AllocateUAVBuffer(const Device* pDevice, Resource* pResource,
				size_t byteWidth, ResourceState dstState = ResourceState::COMMON,
				API api = API::DIRECTX_12);
			static bool AllocateUploadBuffer(const Device* pDevice, Resource* pResource,
				size_t byteWidth, void* pData, API api = API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Bottom-level acceleration structure
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE BottomLevelAS :
			public virtual AccelerationStructure
		{
		public:
			//BottomLevelAS();
			virtual ~BottomLevelAS() {}

			virtual bool Prebuild(const Device* pDevice, uint32_t numGeometries, const GeometryBuffer& geometries,
				BuildFlag flags = BuildFlag::PREFER_FAST_TRACE) = 0;
			virtual void Build(CommandList* pCommandList, const Resource* pScratch,
				const BottomLevelAS* pSource = nullptr, uint8_t numPostbuildInfoDescs = 0,
				const PostbuildInfoType* pPostbuildInfoTypes = nullptr) = 0;

			static void SetTriangleGeometries(GeometryBuffer& geometries, uint32_t numGeometries, Format vertexFormat,
				const VertexBufferView* pVBs, const IndexBufferView* pIBs = nullptr,
				const GeometryFlag* pGeometryFlags = nullptr, const ResourceView* pTransforms = nullptr,
				API api = API::DIRECTX_12);
			static void SetAABBGeometries(GeometryBuffer& geometries, uint32_t numGeometries,
				const VertexBufferView* pVBs, const GeometryFlag* pGeometryFlags = nullptr,
				API api = API::DIRECTX_12);

			using uptr = std::unique_ptr<BottomLevelAS>;
			using sptr = std::shared_ptr<BottomLevelAS>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
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

			virtual bool Prebuild(const Device* pDevice, uint32_t numInstances,
				BuildFlag flags = BuildFlag::PREFER_FAST_TRACE) = 0;
			virtual void Build(CommandList* pCommandList, const Resource* pScratch,
				const Resource* pInstanceDescs, const DescriptorHeap& descriptorHeap,
				const TopLevelAS* pSource = nullptr, uint8_t numPostbuildInfoDescs = 0,
				const PostbuildInfoType* pPostbuildInfoTypes = nullptr) = 0;

			static void SetInstances(const Device* pDevice, Resource* pInstances,
				uint32_t numInstances, const BottomLevelAS* const* ppBottomLevelASs,
				const float* const* transforms, API api = API::DIRECTX_12);
			static void SetInstances(const Device* pDevice, Resource* pInstances,
				uint32_t numInstances, const InstanceDesc* pInstanceDescs,
				API api = API::DIRECTX_12);

			using uptr = std::unique_ptr<TopLevelAS>;
			using sptr = std::shared_ptr<TopLevelAS>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Shader record
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE ShaderRecord
		{
		public:
			//ShaderRecord(const void* pShaderID, uint32_t shaderIDSize,
				//const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0);
			//ShaderRecord(const Device* pDevice, const Pipeline& pipeline, const wchar_t* shaderName,
				//const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0);
			virtual ~ShaderRecord() {}

			virtual void CopyTo(void* dest) const = 0;

			static const void* GetShaderID(const Pipeline& pipeline, const wchar_t* shaderName, API api = API::DIRECTX_12);

			static uint32_t GetShaderIDSize(const Device* pDevice, API api = API::DIRECTX_12);

			using uptr = std::unique_ptr<ShaderRecord>;
			using sptr = std::shared_ptr<ShaderRecord>;

			static uptr MakeUnique(void* pShaderID, uint32_t shaderIDSize, const void* pLocalDescriptorArgs = nullptr,
				uint32_t localDescriptorArgSize = 0, API api = API::DIRECTX_12);
			static sptr MakeShared(void* pShaderID, uint32_t shaderIDSize, const void* pLocalDescriptorArgs = nullptr,
				uint32_t localDescriptorArgSize = 0, API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device* pDevice, const Pipeline& pipeline, const wchar_t* shaderName,
				const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0,
				API api = API::DIRECTX_12); // shader - shader name for DX12
			static sptr MakeShared(const Device* pDevice, const Pipeline& pipeline, const wchar_t* shaderName,
				const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0,
				API api = API::DIRECTX_12); // shader - shader name for DX12
		};

		//--------------------------------------------------------------------------------------
		// Shader table
		//--------------------------------------------------------------------------------------
		class XUSG_INTERFACE ShaderTable
		{
		public:
			//ShaderTable();
			virtual ~ShaderTable() {}

			virtual bool Create(const XUSG::Device* pDevice, uint32_t numShaderRecords, uint32_t shaderRecordSize,
				const wchar_t* name = nullptr) = 0;

			virtual bool AddShaderRecord(const ShaderRecord* pShaderRecord) = 0;

			virtual void* Map() = 0;
			virtual void Unmap() = 0;
			virtual void Reset() = 0;

			virtual const Resource* GetResource() const = 0;
			virtual uint32_t GetShaderRecordSize() const = 0;

			using uptr = std::unique_ptr<ShaderTable>;
			using sptr = std::shared_ptr<ShaderTable>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
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

			virtual bool CreateInterface() = 0;

			virtual void BuildRaytracingAccelerationStructure(const BuildDesc* pDesc,
				uint32_t numPostbuildInfoDescs, const PostbuildInfo* pPostbuildInfoDescs,
				const DescriptorHeap* pDescriptorHeap = nullptr) const = 0;
			virtual void EmitRaytracingAccelerationStructurePostbuildInfo(const PostbuildInfo* pDesc,
				uint32_t numAccelerationStructures, const uint64_t* pAccelerationStructureData) const = 0;
			virtual void CopyRaytracingAccelerationStructure(const AccelerationStructure* pDst,
				const AccelerationStructure* pSrc, CopyMode mode) const = 0;

			virtual void SetDescriptorHeaps(uint32_t numDescriptorHeaps, const DescriptorHeap* pDescriptorHeaps) const = 0;
			virtual void SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const = 0;
			virtual void SetTopLevelAccelerationStructure(uint32_t index, uint64_t topLevelASPtr) const = 0;
			virtual void SetRayTracingPipeline(const Pipeline& pipeline) const = 0;
			virtual void DispatchRays(uint32_t width, uint32_t height, uint32_t depth,
				const ShaderTable* pRayGen, const ShaderTable* pHitGroup, const ShaderTable* pMiss,
				const ShaderTable* pCallable = nullptr) const = 0;
			virtual void DispatchRays(const Pipeline& pipeline, uint32_t width, uint32_t height, uint32_t depth,
				const ShaderTable* pRayGen, const ShaderTable* pHitGroup, const ShaderTable* pMiss,
				const ShaderTable* pCallable = nullptr) const = 0;

			virtual const Device* GetRTDevice() const = 0;

			using uptr = std::unique_ptr<CommandList>;
			using sptr = std::shared_ptr<CommandList>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(XUSG::CommandList* pCommandList, const RayTracing::Device* pDevice, API api = API::DIRECTX_12);
			static sptr MakeShared(XUSG::CommandList* pCommandList, const RayTracing::Device* pDevice, API api = API::DIRECTX_12);
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

			virtual XUSG::PipelineLayout CreatePipelineLayout(const Device* pDevice, PipelineLayoutLib* pPipelineLayoutLib,
				PipelineLayoutFlag flags, const wchar_t* name = nullptr) = 0;
			virtual XUSG::PipelineLayout GetPipelineLayout(const Device* pDevice, PipelineLayoutLib* pPipelineLayoutLib,
				PipelineLayoutFlag flags, const wchar_t* name = nullptr) = 0;

			using uptr = std::unique_ptr<PipelineLayout>;
			using sptr = std::shared_ptr<PipelineLayout>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
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

			virtual Pipeline CreatePipeline(PipelineLib* pPipelineCache, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(PipelineLib* pPipelineCache, const wchar_t* name = nullptr) = 0;

			virtual const std::string& GetKey() = 0;

			virtual const wchar_t* GetHitGroupName(uint32_t index) = 0;
			virtual uint32_t GetNumHitGroups() = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
		};

		class XUSG_INTERFACE PipelineLib
		{
		public:
			//PipelineLib();
			//PipelineLib(const Device* pDevice);
			virtual ~PipelineLib() {}

			virtual void SetDevice(const Device* pDevice) = 0;
			virtual void SetPipeline(const std::string& key, const Pipeline& pipeline) = 0;

			virtual Pipeline CreatePipeline(State* pState, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(State* pState, const wchar_t* name = nullptr) = 0;

			using uptr = std::unique_ptr<PipelineLib>;
			using sptr = std::shared_ptr<PipelineLib>;

			static uptr MakeUnique(API api = API::DIRECTX_12);
			static sptr MakeShared(API api = API::DIRECTX_12);
			static uptr MakeUnique(const Device* pDevice, API api = API::DIRECTX_12);
			static sptr MakeShared(const Device* pDevice, API api = API::DIRECTX_12);
		};
	}
}
