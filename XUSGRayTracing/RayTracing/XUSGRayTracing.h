//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"

namespace XUSG
{
	namespace RayTracing
	{
		enum class BuildFlags
		{
			NONE = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE,
			ALLOW_UPDATE = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE,
			ALLOW_COMPACTION = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION,
			PREFER_FAST_TRACE = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
			PREFER_FAST_BUILD = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD,
			MINIMIZE_MEMORY = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY,
			PERFORM_UPDATE = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE
		};

		DEFINE_ENUM_FLAG_OPERATORS(BuildFlags);

		enum class GeometryFlags
		{
			NONE = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE,
			FULL_OPAQUE = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
			NO_DUPLICATE_ANYHIT_INVOCATION = D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION
		};

		DEFINE_ENUM_FLAG_OPERATORS(GeometryFlags);

		enum class HitGroupType : uint8_t
		{
			TRIANGLES = D3D12_HIT_GROUP_TYPE_TRIANGLES,
			PROCEDURAL = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE
		};

		using BuildDesc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC;
		using Geometry = D3D12_RAYTRACING_GEOMETRY_DESC;
		using PipilineDesc = CD3D12_STATE_OBJECT_DESC;

		struct PrebuildInfo
		{
			uint64_t ResultDataMaxSizeInBytes;
			uint64_t ScratchDataSizeInBytes;
			uint64_t UpdateScratchDataSizeInBytes;
		};

		struct PostbuildInfo
		{
			uint64_t DestBuffer;
			uint32_t InfoType;
		};

		struct ResourceView
		{
			const Resource* pResource;
			uint64_t Offset;
		};

		//--------------------------------------------------------------------------------------
		// Device
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE Device :
			public virtual XUSG::Device
		{
		public:
			//Device();
			virtual ~Device() {};

#if ENABLE_DXR_FALLBACK
			virtual bool CreateInterface(uint8_t flags) = 0;
#else
			virtual bool CreateInterface() = 0;
#endif

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

		class DLL_INTERFACE AccelerationStructure
		{
		public:
			//AccelerationStructure();
			virtual ~AccelerationStructure() {}

			virtual RawBuffer::sptr GetResult() const = 0;

			virtual uint32_t GetResultDataMaxSize() const = 0;
			virtual uint32_t GetScratchDataMaxSize() const = 0;
			virtual uint32_t GetUpdateScratchDataSize() const = 0;
#if ENABLE_DXR_FALLBACK
			virtual const WRAPPED_GPU_POINTER& GetResultPointer() const = 0;

			static uint32_t GetUAVCount();

			static void SetUAVCount(uint32_t numUAVs);
#endif
			static void SetFrameCount(uint32_t frameCount);

			static bool AllocateUAVBuffer(const Device* pDevice, Resource* pResource,
				size_t byteWidth, ResourceState dstState = ResourceState::UNORDERED_ACCESS,
				XUSG::API api = XUSG::API::DIRECTX_12);
			static bool AllocateUploadBuffer(const Device* pDevice, Resource* pResource,
				size_t byteWidth, void* pData, XUSG::API api = XUSG::API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Bottom-level acceleration structure
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE BottomLevelAS :
			public virtual AccelerationStructure
		{
		public:
			//BottomLevelAS();
			virtual ~BottomLevelAS() {}

			virtual bool PreBuild(const Device* pDevice, uint32_t numDescs, const Geometry* pGeometries,
				uint32_t descriptorIndex, BuildFlags flags = BuildFlags::PREFER_FAST_TRACE) = 0;
			virtual void Build(const CommandList* pCommandList, const Resource* pScratch,
				const DescriptorPool& descriptorPool, bool update = false) = 0;
#if !ENABLE_DXR_FALLBACK
			virtual void Build(XUSG::CommandList* pCommandList, const Resource* pScratch,
				const DescriptorPool& descriptorPool, bool update = false) = 0;
#endif

			static void SetTriangleGeometries(Geometry* pGeometries, uint32_t numGeometries, Format vertexFormat,
				const VertexBufferView* pVBs, const IndexBufferView* pIBs = nullptr,
				const GeometryFlags* pGeometryFlags = nullptr, const ResourceView* pTransforms = nullptr,
				XUSG::API api = XUSG::API::DIRECTX_12);
			static void SetAABBGeometries(Geometry* pGeometries, uint32_t numGeometries,
				const VertexBufferView* pVBs, const GeometryFlags* pGeometryFlags = nullptr,
				XUSG::API api = XUSG::API::DIRECTX_12);

			using uptr = std::unique_ptr<BottomLevelAS>;
			using sptr = std::shared_ptr<BottomLevelAS>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Top-level acceleration structure
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE TopLevelAS :
			public virtual AccelerationStructure
		{
		public:
			//TopLevelAS();
			virtual ~TopLevelAS() {}

			virtual bool PreBuild(const Device* pDevice, uint32_t numDescs, uint32_t descriptorIndex,
				BuildFlags flags = BuildFlags::PREFER_FAST_TRACE) = 0;
			virtual void Build(const CommandList* pCommandList, const Resource* pScratch,
				const Resource* pInstanceDescs, const DescriptorPool& descriptorPool, bool update = false) = 0;
#if !ENABLE_DXR_FALLBACK
			virtual void Build(XUSG::CommandList* pCommandList, const Resource* pScratch,
				const Resource* pInstanceDescs, const DescriptorPool& descriptorPool, bool update = false) = 0;
#endif

			static void SetInstances(const Device* pDevice, Resource* pInstances,
				uint32_t numInstances, const BottomLevelAS* const* ppBottomLevelASs,
				float* const* transforms, XUSG::API api = XUSG::API::DIRECTX_12);

			using uptr = std::unique_ptr<TopLevelAS>;
			using sptr = std::shared_ptr<TopLevelAS>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Shader record
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE ShaderRecord
		{
		public:
			//ShaderRecord(const Device* pDevice, const Pipeline& pipeline, const void* shader,
				//const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0);
			//ShaderRecord(void* pShaderID, uint32_t shaderIDSize,
				//const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0);
			virtual ~ShaderRecord() {}

			virtual void CopyTo(void* dest) const = 0;

			static uint32_t GetShaderIDSize(const Device* pDevice, XUSG::API api = XUSG::API::DIRECTX_12);

			using uptr = std::unique_ptr<ShaderRecord>;
			using sptr = std::shared_ptr<ShaderRecord>;

			static uptr MakeUnique(const Device* pDevice, const Pipeline& pipeline, const void* shader,
				const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0,
				XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(const Device* pDevice, const Pipeline& pipeline, const void* shader,
				const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0,
				XUSG::API api = XUSG::API::DIRECTX_12);
			static uptr MakeUnique(void* pShaderID, uint32_t shaderIDSize, const void* pLocalDescriptorArgs = nullptr,
				uint32_t localDescriptorArgSize = 0, XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(void* pShaderID, uint32_t shaderIDSize, const void* pLocalDescriptorArgs = nullptr,
				uint32_t localDescriptorArgSize = 0, XUSG::API api = XUSG::API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Shader table
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE ShaderTable
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

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Command list
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE CommandList :
			public virtual XUSG::CommandList
		{
		public:
			//CommandList();
			virtual ~CommandList() {}

#if ENABLE_DXR_FALLBACK
			virtual bool CreateInterface(const Device* pDevice) = 0;
#else
			virtual bool CreateInterface() = 0;
#endif

			virtual void BuildRaytracingAccelerationStructure(const BuildDesc* pDesc,
				uint32_t numPostbuildInfoDescs,
				const PostbuildInfo* pPostbuildInfoDescs,
				const DescriptorPool& descriptorPool) const = 0;

			virtual void SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const = 0;
			virtual void SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS* pTopLevelAS) const = 0;
			virtual void DispatchRays(const Pipeline& pipeline, uint32_t width, uint32_t height, uint32_t depth,
				const ShaderTable* pHitGroup, const ShaderTable* pMiss, const ShaderTable* pRayGen) const = 0;

			using uptr = std::unique_ptr<CommandList>;
			using sptr = std::shared_ptr<CommandList>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
#if ENABLE_DXR_FALLBACK
			static uptr MakeUnique(XUSG::CommandList* pCommandList, const RayTracing::Device* pDevice, XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::CommandList* pCommandList, const RayTracing::Device* pDevice, XUSG::API api = XUSG::API::DIRECTX_12);
#else
			static uptr MakeUnique(XUSG::CommandList* pCommandList, XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::CommandList* pCommandList, XUSG::API api = XUSG::API::DIRECTX_12);
#endif
		};

		//--------------------------------------------------------------------------------------
		// Pipeline layout
		//--------------------------------------------------------------------------------------
		class DLL_INTERFACE PipelineLayout :
			public virtual Util::PipelineLayout
		{
		public:
			//PipelineLayout();
			virtual ~PipelineLayout() {}

			virtual XUSG::PipelineLayout CreatePipelineLayout(const Device* pDevice, PipelineLayoutCache* pPipelineLayoutCache,
				PipelineLayoutFlag flags, const wchar_t* name = nullptr) = 0;
			virtual XUSG::PipelineLayout GetPipelineLayout(const Device* pDevice, PipelineLayoutCache* pPipelineLayoutCache,
				PipelineLayoutFlag flags, const wchar_t* name = nullptr) = 0;

			using uptr = std::unique_ptr<PipelineLayout>;
			using sptr = std::shared_ptr<PipelineLayout>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
		};

		//--------------------------------------------------------------------------------------
		// Pipeline state
		//--------------------------------------------------------------------------------------
		class PipelineCache;

		class DLL_INTERFACE State
		{
		public:
			//State();
			virtual ~State() {}

			virtual void SetShaderLibrary(Blob shaderLib) = 0;
			virtual void SetHitGroup(uint32_t index, const void* hitGroup, const void* closestHitShader,
				const void* anyHitShader = nullptr, const void* intersectionShader = nullptr,
				HitGroupType type = HitGroupType::TRIANGLES) = 0;
			virtual void SetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize) = 0;
			virtual void SetLocalPipelineLayout(uint32_t index, const XUSG::PipelineLayout& layout,
				uint32_t numShaders, const void** pShaders) = 0;
			virtual void SetGlobalPipelineLayout(const XUSG::PipelineLayout& layout) = 0;
			virtual void SetMaxRecursionDepth(uint32_t depth) = 0;

			virtual Pipeline CreatePipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(PipelineCache& pipelineCache, const wchar_t* name = nullptr) = 0;

			virtual const std::string& GetKey() = 0;

			using uptr = std::unique_ptr<State>;
			using sptr = std::shared_ptr<State>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
		};

		class DLL_INTERFACE PipelineCache
		{
		public:
			//PipelineCache();
			//PipelineCache(const Device* pDevice);
			virtual ~PipelineCache() {}

			virtual void SetDevice(const Device* pDevice) = 0;
			virtual void SetPipeline(const std::string& key, const Pipeline& pipeline) = 0;

			virtual Pipeline CreatePipeline(State& state, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(State& state, const wchar_t* name = nullptr) = 0;

			using uptr = std::unique_ptr<PipelineCache>;
			using sptr = std::shared_ptr<PipelineCache>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
			static uptr MakeUnique(const Device* pDevice, XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(const Device* pDevice, XUSG::API api = XUSG::API::DIRECTX_12);
		};
	}
}
