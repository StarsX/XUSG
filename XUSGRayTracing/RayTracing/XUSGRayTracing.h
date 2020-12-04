//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing_DX12.h"

namespace XUSG
{
	namespace RayTracing
	{
		struct ResourceView
		{
			Resource resource;
			uint64_t offset;
		};

		//--------------------------------------------------------------------------------------
		// Acceleration structure
		//--------------------------------------------------------------------------------------
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

			static bool AllocateUAVBuffer(const Device& device, Resource& resource,
				size_t byteWidth, ResourceState dstState = ResourceState::UNORDERED_ACCESS,
				XUSG::API api = XUSG::API::DIRECTX_12);
			static bool AllocateUploadBuffer(const Device& device, Resource& resource,
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

			virtual bool PreBuild(const Device& device, uint32_t numDescs, const Geometry* pGeometries,
				uint32_t descriptorIndex, BuildFlags flags = BuildFlags::PREFER_FAST_TRACE) = 0;
			virtual void Build(const CommandList* pCommandList, const Resource& scratch,
				const DescriptorPool& descriptorPool, bool update = false) = 0;
#if !ENABLE_DXR_FALLBACK
			virtual void Build(XUSG::CommandList* pCommandList, const Resource& scratch,
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

			virtual bool PreBuild(const Device& device, uint32_t numDescs, uint32_t descriptorIndex,
				BuildFlags flags = BuildFlags::PREFER_FAST_TRACE) = 0;
			virtual void Build(const CommandList* pCommandList, const Resource& scratch,
				const Resource& instanceDescs, const DescriptorPool& descriptorPool, bool update = false) = 0;
#if !ENABLE_DXR_FALLBACK
			virtual void Build(XUSG::CommandList* pCommandList, const Resource& scratch,
				const Resource& instanceDescs, const DescriptorPool& descriptorPool, bool update = false) = 0;
#endif

			static void SetInstances(const Device& device, Resource& instances,
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
			//ShaderRecord(const Device& device, const Pipeline& pipeline, const void* shader,
				//const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0);
			//ShaderRecord(void* pShaderID, uint32_t shaderIDSize,
				//const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0);
			virtual ~ShaderRecord() {}

			virtual void CopyTo(void* dest) const = 0;

			static uint32_t GetShaderIDSize(const Device& device, XUSG::API api = XUSG::API::DIRECTX_12);

			using uptr = std::unique_ptr<ShaderRecord>;
			using sptr = std::shared_ptr<ShaderRecord>;

			static uptr MakeUnique(const Device& device, const Pipeline& pipeline, const void* shader,
				const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0,
				XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(const Device& device, const Pipeline& pipeline, const void* shader,
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

			virtual bool Create(const XUSG::Device& device, uint32_t numShaderRecords, uint32_t shaderRecordSize,
				const wchar_t* name = nullptr) = 0;

			virtual bool AddShaderRecord(const ShaderRecord& shaderRecord) = 0;

			virtual void* Map() = 0;
			virtual void Unmap() = 0;
			virtual void Reset() = 0;

			virtual const Resource& GetResource() const = 0;
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
			virtual bool CreateInterface(const Device& device) = 0;
#else
			virtual bool CreateInterface() = 0;
#endif

			virtual void BuildRaytracingAccelerationStructure(const BuildDesc* pDesc,
				uint32_t numPostbuildInfoDescs,
				const PostbuildInfo* pPostbuildInfoDescs,
				const DescriptorPool& descriptorPool) const = 0;

			virtual void SetDescriptorPools(uint32_t numDescriptorPools, const DescriptorPool* pDescriptorPools) const = 0;
			virtual void SetTopLevelAccelerationStructure(uint32_t index, const TopLevelAS& topLevelAS) const = 0;
			virtual void DispatchRays(const Pipeline& pipeline, uint32_t width, uint32_t height, uint32_t depth,
				const ShaderTable& hitGroup, const ShaderTable& miss, const ShaderTable& rayGen) const = 0;

			using uptr = std::unique_ptr<CommandList>;
			using sptr = std::shared_ptr<CommandList>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
#if ENABLE_DXR_FALLBACK
			static uptr MakeUnique(XUSG::CommandList& commandList, const RayTracing::Device& device, XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::CommandList& commandList, const RayTracing::Device& device, XUSG::API api = XUSG::API::DIRECTX_12);
#else
			static uptr MakeUnique(XUSG::CommandList& commandList, XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::CommandList& commandList, XUSG::API api = XUSG::API::DIRECTX_12);
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

			virtual XUSG::PipelineLayout CreatePipelineLayout(const Device& device, PipelineLayoutCache& pipelineLayoutCache,
				PipelineLayoutFlag flags, const wchar_t* name = nullptr) = 0;
			virtual XUSG::PipelineLayout GetPipelineLayout(const Device& device, PipelineLayoutCache& pipelineLayoutCache,
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
			//PipelineCache(const Device& device);
			virtual ~PipelineCache() {}

			virtual void SetDevice(const Device& device) = 0;
			virtual void SetPipeline(const std::string& key, const Pipeline& pipeline) = 0;

			virtual Pipeline CreatePipeline(State& state, const wchar_t* name = nullptr) = 0;
			virtual Pipeline GetPipeline(State& state, const wchar_t* name = nullptr) = 0;

			using uptr = std::unique_ptr<PipelineCache>;
			using sptr = std::shared_ptr<PipelineCache>;

			static uptr MakeUnique(XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(XUSG::API api = XUSG::API::DIRECTX_12);
			static uptr MakeUnique(const Device& device, XUSG::API api = XUSG::API::DIRECTX_12);
			static sptr MakeShared(const Device& device, XUSG::API api = XUSG::API::DIRECTX_12);
		};
	}
}
