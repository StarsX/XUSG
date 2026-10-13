//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG.h"

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>
#include <mutex>

#define XUSG_VK_CHECK(x, o, r)	{ const auto vr = (x); if (vr < 0) { o << "VkResult " << vr << std::endl; assert(vr >= 0); return r; } }

#if defined(WIN32) || defined(_WIN32)
#define XUSG_SIGNAL_EVENT(event) SetEvent(static_cast<HANDLE>(event))
#else
#define XUSG_SIGNAL_EVENT(event) do { (void)(event); assert(!"Fence event is not implemented on this platform"); } while (0)
#endif

namespace XUSG
{
	struct Blob_VK
	{
		std::vector<uint8_t> Data;
	};

	struct RootParameter_VK
	{
		DescriptorType Type;
		uint32_t PushByteOffset;
		uint32_t SizeInBytes;
		Shader::Stage Stage;
	};

	struct EmbeddedSampler_VK
	{
		VkSamplerCreateInfo Info;
		VkSamplerReductionModeCreateInfo Reduction;
		VkSamplerCustomBorderColorCreateInfoEXT Border;
	};

	// XUSG pipeline layout for VK_EXT_descriptor_heap. Pipelines are created with a null
	// VkPipelineLayout. This object stores the descriptor mapping and the push-data layout.
	struct PipelineLayoutObject
	{
		PipelineLayoutObject();

		PipelineLayoutObject(const PipelineLayoutObject&) = delete;
		PipelineLayoutObject& operator=(const PipelineLayoutObject&) = delete;

		std::vector<VkDescriptorSetAndBindingMappingEXT> StageMappings[Shader::NUM_STAGE];
		VkShaderDescriptorSetAndBindingMappingInfoEXT StageMappingInfos[Shader::NUM_STAGE];
		std::vector<RootParameter_VK> RootParameters;
		std::vector<EmbeddedSampler_VK> EmbeddedSamplers;
		uint32_t PushDataSize;
	};

	struct Pipeline_VK
	{
		VkPipeline Handle;
		VkPipelineBindPoint BindPoint;
		Blob_VK Cache;
		uint32_t Refs;
		uint32_t XfbBuffers;
	};

	void RetainVKPipeline(Pipeline_VK* pPipeline);
	void ReleaseVKPipeline(VkDevice device, Pipeline_VK* pPipeline);

	class Device_VK;

	struct QueueNative
	{
		VkQueue Queue;
		Device_VK* Device;
		uint32_t Family;
	};

	void RegisterQueue(const QueueNative& queue);
	void UnregisterQueue(VkQueue queue);
	bool FindQueue(VkQueue queue, QueueNative& native);
	void SetQueueAcquireWait(VkQueue queue, VkSemaphore wait, VkFence fence, VkSemaphore presentSignal);
	bool TakeQueueAcquireWait(VkQueue queue, VkSemaphore& wait, VkFence& fence, VkSemaphore& presentSignal);
	void ClearQueueAcquireWait(VkQueue queue);

	// Pending present flushed from CommandQueue::Signal (vkd3d-style).
	class SwapChain_VK;
	struct QueuePendingPresent
	{
		SwapChain_VK* SwapChain;
		uint32_t UserIndex;
	};
	void SetQueuePendingPresent(VkQueue queue, const QueuePendingPresent& present);
	bool TakeQueuePendingPresent(VkQueue queue, QueuePendingPresent& present);
	bool HasQueuePendingPresent(VkQueue queue);
	bool FlushQueuePendingPresent(VkQueue queue);

	class Device_VK :
		public virtual Device
	{
	public:
		Device_VK();
		virtual ~Device_VK();

		bool GetCommandQueue(CommandQueue* pCommandQueue, CommandListType type,
			CommandQueueFlag flags, int32_t priority = 0, uint32_t nodeMask = 0,
			const wchar_t* name = nullptr);
		bool GetCommandAllocator(CommandAllocator* pCommandAllocator,
			CommandListType type, const wchar_t* name = nullptr);
		bool GetCommandList(CommandList* pCommandList, uint32_t nodeMask, CommandListType type,
			CommandAllocator* pCommandAllocator, const Pipeline& pipeline,
			const wchar_t* name = nullptr);
		bool GetFence(Fence* pFence, uint64_t initialValue, FenceFlag flags,
			const wchar_t* name = nullptr);

		uint32_t Create(void* pAdapter, uint32_t minFeatureLevel, const wchar_t* name = nullptr);
		uint32_t GetDeviceRemovedReason() const;

		void Create(void* pNativeHandle);

		void* GetNativeHandle();
		void* GetAdapterNativeHandle();
		void* GetBackend() override;

		VkInstance GetInstance() const;
		VkPhysicalDevice GetPhysicalDevice() const;
		VkDevice GetDevice() const;
		uint32_t GetApiVersion() const;
		uint32_t GetGraphicsFamily() const;
		uint32_t GetQueueFamily(CommandListType type) const;
		void GetSharingFamilies(uint32_t& count, const uint32_t*& pFamilies) const;
		bool AcquireQueue(CommandListType type, uint32_t& family, uint32_t& queueIndex) const;
		void ReleaseQueue(uint32_t family, uint32_t queueIndex) const;

		uint32_t GetResourceDescriptorStride() const;
		uint32_t GetSamplerDescriptorStride() const;
		const VkPhysicalDeviceDescriptorHeapPropertiesEXT& GetDescriptorHeapProperties() const;
		const VkPhysicalDeviceDeviceGeneratedCommandsPropertiesEXT& GetGeneratedCommandsProperties() const;

		PFN_vkCmdBeginRendering GetBeginRendering() const;
		PFN_vkCmdEndRendering GetEndRendering() const;
		PFN_vkCmdSetPrimitiveTopology GetSetPrimitiveTopology() const;
		PFN_vkCmdBindVertexBuffers2 GetBindVertexBuffers2() const;

		struct ExtProcs
		{
			PFN_vkSetDebugUtilsObjectNameEXT SetDebugName;
			PFN_vkCreateIndirectCommandsLayoutEXT CreateIndirectCommandsLayout;
			PFN_vkDestroyIndirectCommandsLayoutEXT DestroyIndirectCommandsLayout;
			PFN_vkCmdBindSamplerHeapEXT BindSamplerHeap;
			PFN_vkCmdBindResourceHeapEXT BindResourceHeap;
			PFN_vkCmdPushDataEXT PushData;
			PFN_vkGetGeneratedCommandsMemoryRequirementsEXT GetGeneratedCommandsMemoryRequirements;
			PFN_vkCmdPreprocessGeneratedCommandsEXT PreprocessGeneratedCommands;
			PFN_vkCmdExecuteGeneratedCommandsEXT ExecuteGeneratedCommands;
			PFN_vkCmdTraceRaysIndirect2KHR TraceRaysIndirect2;
			PFN_vkCreateAccelerationStructureKHR CreateAccelerationStructure;
			PFN_vkDestroyAccelerationStructureKHR DestroyAccelerationStructure;
			PFN_vkGetAccelerationStructureBuildSizesKHR GetAccelerationStructureBuildSizes;
			PFN_vkGetAccelerationStructureDeviceAddressKHR GetAccelerationStructureDeviceAddress;
			PFN_vkCmdBuildAccelerationStructuresKHR CmdBuildAccelerationStructures;
			PFN_vkCmdCopyAccelerationStructureKHR CmdCopyAccelerationStructure;
			PFN_vkCmdWriteAccelerationStructuresPropertiesKHR CmdWriteAccelerationStructuresProperties;
			PFN_vkCreateRayTracingPipelinesKHR CreateRayTracingPipelines;
			PFN_vkGetRayTracingShaderGroupHandlesKHR GetRayTracingShaderGroupHandles;
			PFN_vkCmdTraceRaysKHR CmdTraceRays;
			PFN_vkWriteSamplerDescriptorsEXT WriteSamplerDescriptors;
			PFN_vkWriteResourceDescriptorsEXT WriteResourceDescriptors;
			PFN_vkCmdBeginConditionalRenderingEXT BeginConditionalRendering;
			PFN_vkCmdEndConditionalRenderingEXT EndConditionalRendering;
			PFN_vkCmdBindTransformFeedbackBuffersEXT BindTransformFeedbackBuffers;
			PFN_vkCmdBeginTransformFeedbackEXT BeginTransformFeedback;
			PFN_vkCmdEndTransformFeedbackEXT EndTransformFeedback;
			PFN_vkCmdBeginQueryIndexedEXT BeginQueryIndexed;
			PFN_vkCmdEndQueryIndexedEXT EndQueryIndexed;
			PFN_vkCmdSetSampleLocationsEXT SetSampleLocations;
			PFN_vkCmdSetSampleLocationsEnableEXT SetSampleLocationsEnable;
			PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT GetMultisampleProperties;
		};

		const ExtProcs& GetExtProcs() const;

		bool HasDebugUtils() const;
		bool HasCustomBorderColor() const;
		bool HasVertexAttributeDivisor() const;
		bool HasShaderUntypedPointers() const;
		bool HasNullDescriptor() const;
		bool HasDeviceGeneratedCommands() const;
		bool HasMaintenance5() const;
		bool HasSynchronization2() const;
		bool HasDepthStencilResolve() const;
		VkResolveModeFlags SupportedDepthResolveModes() const;
		VkResolveModeFlags SupportedStencilResolveModes() const;
		bool IndependentDepthStencilResolveNone() const;
		bool IndependentDepthStencilResolve() const;
		bool HasShaderInt64() const;
		bool HasShaderInt8() const;
		bool HasStorageBuffer8BitAccess() const;
		bool HasUniformAndStorageBuffer8BitAccess() const;
		bool HasStoragePushConstant8() const;
		bool HasStorageBuffer16BitAccess() const;
		bool HasUniformAndStorageBuffer16BitAccess() const;
		bool HasStoragePushConstant16() const;
		bool HasStorageInputOutput16() const;
		bool HasShaderDrawParameters() const;
		bool HasShaderSubgroupExtendedTypes() const;
		bool HasShaderBufferInt64Atomics() const;
		bool HasShaderSharedInt64Atomics() const;
		bool HasShaderImageInt64Atomics() const;
		bool HasSparseImageInt64Atomics() const;
		bool HasSubgroupSizeControl() const;
		bool HasComputeFullSubgroups() const;
		bool HasShaderIntegerDotProduct() const;
		bool HasShaderDemoteToHelperInvocation() const;
		bool HasShaderSubgroupRotate() const;
		bool HasShaderSubgroupRotateClustered() const;
		bool HasShaderFloatControls2() const;
		bool HasShaderSubgroupPartitioned() const;
		bool HasShaderQuadControl() const;
		bool HasShaderSubgroupUniformControlFlow() const;
		bool HasShaderMaximalReconvergence() const;
		bool HasComputeDerivativeGroupQuads() const;
		bool HasComputeDerivativeGroupLinear() const;
		bool HasFragmentShaderBarycentric() const;
		bool HasShaderSubgroupBallot() const;
		bool HasShaderSubgroupVote() const;
		bool HasShaderUniformBufferArrayDynamicIndexing() const;
		bool HasShaderSampledImageArrayDynamicIndexing() const;
		bool HasShaderStorageBufferArrayDynamicIndexing() const;
		bool HasShaderStorageImageArrayDynamicIndexing() const;
		bool HasShaderInputAttachmentArrayDynamicIndexing() const;
		bool HasShaderUniformTexelBufferArrayDynamicIndexing() const;
		bool HasShaderStorageTexelBufferArrayDynamicIndexing() const;
		bool HasShaderUniformBufferArrayNonUniformIndexing() const;
		bool HasShaderSampledImageArrayNonUniformIndexing() const;
		bool HasShaderStorageBufferArrayNonUniformIndexing() const;
		bool HasShaderStorageImageArrayNonUniformIndexing() const;
		bool HasShaderInputAttachmentArrayNonUniformIndexing() const;
		bool HasShaderUniformTexelBufferArrayNonUniformIndexing() const;
		bool HasShaderStorageTexelBufferArrayNonUniformIndexing() const;
		bool HasShaderInt16() const;
		bool HasShaderFloat64() const;
		bool HasShaderStorageImageReadWithoutFormat() const;
		bool HasShaderStorageImageExtendedFormats() const;
		bool HasShaderStorageImageMultisample() const;
		bool HasImageFormatList() const;
		bool HasShaderClipDistance() const;
		bool HasShaderCullDistance() const;
		bool HasVertexPipelineStoresAndAtomics() const;
		bool HasSampleRateShading() const;
		bool HasImageCubeArray() const;
		bool HasTextureCompressionBC() const;
		bool HasDrawIndirectFirstInstance() const;
		bool HasScalarBlockLayout() const;
		bool HasRuntimeDescriptorArray() const;
		bool HasDescriptorBindingPartiallyBound() const;
		bool HasShaderOutputViewportIndex() const;
		bool HasShaderOutputLayer() const;
		bool HasSubgroupBroadcastDynamicId() const;
		bool HasShaderBufferFloat32Atomics() const;
		bool HasShaderBufferFloat32AtomicAdd() const;
		bool HasShaderBufferFloat64Atomics() const;
		bool HasShaderBufferFloat64AtomicAdd() const;
		bool HasShaderSharedFloat32Atomics() const;
		bool HasShaderSharedFloat32AtomicAdd() const;
		bool HasShaderSharedFloat64Atomics() const;
		bool HasShaderSharedFloat64AtomicAdd() const;
		bool HasShaderImageFloat32Atomics() const;
		bool HasShaderImageFloat32AtomicAdd() const;
		bool HasSparseImageFloat32Atomics() const;
		bool HasSparseImageFloat32AtomicAdd() const;
		bool HasShaderBufferFloat16Atomics() const;
		bool HasShaderBufferFloat16AtomicAdd() const;
		bool HasShaderBufferFloat16AtomicMinMax() const;
		bool HasShaderBufferFloat32AtomicMinMax() const;
		bool HasShaderBufferFloat64AtomicMinMax() const;
		bool HasShaderSharedFloat16Atomics() const;
		bool HasShaderSharedFloat16AtomicAdd() const;
		bool HasShaderSharedFloat16AtomicMinMax() const;
		bool HasShaderSharedFloat32AtomicMinMax() const;
		bool HasShaderSharedFloat64AtomicMinMax() const;
		bool HasShaderImageFloat32AtomicMinMax() const;
		bool HasSparseImageFloat32AtomicMinMax() const;
		bool HasShaderSubgroupClock() const;
		bool HasShaderDeviceClock() const;
		bool HasHlslFunctionality1() const;
		bool HasDeferredHostOperations() const;
		bool HasPipelineLibrary() const;
		bool HasSpirv14() const;
		bool HasShaderFloatControls() const;
		bool HasAccelerationStructure() const;
		bool HasAccelerationStructureCaptureReplay() const;
		bool HasAccelerationStructureIndirectBuild() const;
		bool HasAccelerationStructureHostCommands() const;
		bool HasRayTracingPipeline() const;
		bool HasRayTracingPipelineShaderGroupHandleCaptureReplay() const;
		bool HasRayTracingPipelineShaderGroupHandleCaptureReplayMixed() const;
		bool HasRayTracingPipelineTraceRaysIndirect() const;
		bool HasRayTraversalPrimitiveCulling() const;
		bool HasRayQuery() const;
		bool HasRayTracingMaintenance1() const;
		bool HasRayTracingPipelineTraceRaysIndirect2() const;
		const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetRayTracingPipelineProperties() const;
		uint32_t GetShaderGroupHandleSize() const;
		uint32_t GetShaderGroupHandleAlignment() const;
		uint32_t GetShaderGroupBaseAlignment() const;
		uint32_t GetMaxRayRecursionDepth() const;
		bool HasRayTracingPositionFetch() const;
		bool HasMicromap() const;
		bool HasOpacityMicromapKhr() const;
		bool HasMicromapCaptureReplay() const;
		bool HasMicromapHostCommands() const;
		bool HasIndexTypeUint8() const;
		bool HasRayTracingInvocationReorder() const;
		bool HasPipelineLibraryGroupHandles() const;
		bool HasCooperativeMatrix() const;
		bool HasCooperativeMatrixRobustBufferAccess() const;
		bool HasShaderBFloat16Type() const;
		bool HasShaderBFloat16DotProduct() const;
		bool HasShaderBFloat16CooperativeMatrix() const;
		bool HasShaderFloat8() const;
		bool HasShaderFloat8CooperativeMatrix() const;
		bool HasWorkgroupMemoryExplicitLayout() const;
		bool HasWorkgroupMemoryExplicitLayoutScalarBlockLayout() const;
		bool HasWorkgroupMemoryExplicitLayout8BitAccess() const;
		bool HasWorkgroupMemoryExplicitLayout16BitAccess() const;
		bool HasLongVector() const;
		uint32_t MaxLongVectorComponents() const;
		bool HasCooperativeMatrixProperties2() const;
		bool HasCooperativeMatrixReductions() const;
		bool HasCooperativeMatrixConversions() const;
		bool HasCooperativeMatrixPerElementOperations() const;
		bool HasCooperativeMatrixGetCoordinate() const;
		uint32_t SubgroupSize() const;
		VkShaderStageFlags SubgroupStages() const;
		VkSubgroupFeatureFlags SubgroupOperations() const;
		bool SubgroupQuadOperationsInAllStages() const;
		bool HasMeshShader() const;
		bool HasFragmentShadingRate() const;
		bool HasConditionalRendering() const;
		bool HasTransformFeedback() const;
		bool HasGeometryStreams() const;
		bool HasSampleLocations() const;
		bool VariableSampleLocations() const;
		bool SampleLocationGrid(uint32_t sampleCount, uint32_t& width, uint32_t& height) const;
		void SampleLocationRange(float& minimum, float& maximum, uint32_t& subPixelBits) const;
		bool HasConservativeRasterization() const;
		bool HasSparseBinding() const;
		bool SupportsSparseBuffer() const;
		bool SupportsSparseImage(bool is3D, uint32_t sampleCount) const;
		bool SupportsSparseAliased() const;
		bool GetSparseQueue(VkQueue& queue) const;
		bool QueueSparseBind(VkQueue queue, VkBindSparseInfo bind) const;
		bool SubmitQueue(VkQueue queue, const VkSubmitInfo2& info, VkFence fence,
			VkSemaphore* pWaitSemaphores, VkPipelineStageFlags* pWaitStages, uint64_t* pWaitValues,
			VkCommandBuffer* pCommandBuffers, VkSemaphore* pSignalSemaphores, uint64_t* pSignalValues) const;
		void ReadSparseTimeline(VkSemaphore& semaphore, uint64_t& value) const;
		bool EmulatesD24S8() const;

		uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
		void SetDebugName(VkObjectType type, uint64_t handle, const wchar_t* name) const;

	protected:
		bool createInstance();
		bool pickPhysicalDevice(VkPhysicalDevice requested);
		bool createDevice();

		VkInstance			m_instance;
		VkPhysicalDevice	m_physicalDevice;
		VkDevice			m_device;

		uint32_t			m_apiVersion;
		uint32_t			m_graphicsFamily;
		uint32_t			m_resourceStride;
		uint32_t			m_samplerStride;
		uint32_t			m_removedReason;

		bool				m_debugUtils;
		bool				m_customBorderColor;
		bool				m_vertexAttributeDivisor;
		bool				m_shaderUntypedPointers;
		bool				m_nullDescriptor;
		bool				m_deviceGeneratedCommands;
		bool				m_maintenance5;
		bool				m_synchronization2;
		bool				m_depthStencilResolve;
		bool				m_independentResolveNone;
		bool				m_independentResolve;
		VkResolveModeFlags	m_depthResolveModes;
		VkResolveModeFlags	m_stencilResolveModes;
		bool				m_shaderInt64;
		bool				m_shaderInt8;
		bool				m_storageBuffer8BitAccess;
		bool				m_uniformAndStorageBuffer8BitAccess;
		bool				m_storagePushConstant8;
		bool				m_storageBuffer16BitAccess;
		bool				m_uniformAndStorageBuffer16BitAccess;
		bool				m_storagePushConstant16;
		bool				m_storageInputOutput16;
		bool				m_shaderDrawParameters;
		bool				m_shaderSubgroupExtendedTypes;
		bool				m_shaderBufferInt64Atomics;
		bool				m_shaderSharedInt64Atomics;
		bool				m_shaderImageInt64Atomics;
		bool				m_sparseImageInt64Atomics;
		bool				m_subgroupSizeControl;
		bool				m_computeFullSubgroups;
		bool				m_shaderIntegerDotProduct;
		bool				m_shaderDemoteToHelperInvocation;
		bool				m_shaderSubgroupRotate;
		bool				m_shaderSubgroupRotateClustered;
		bool				m_shaderFloatControls2;
		bool				m_shaderSubgroupPartitioned;
		bool				m_shaderQuadControl;
		bool				m_shaderSubgroupUniformControlFlow;
		bool				m_shaderMaximalReconvergence;
		bool				m_computeDerivativeGroupQuads;
		bool				m_computeDerivativeGroupLinear;
		bool				m_fragmentShaderBarycentric;
		bool				m_shaderSubgroupBallot;
		bool				m_shaderSubgroupVote;
		bool				m_shaderUniformBufferArrayDynamicIndexing;
		bool				m_shaderSampledImageArrayDynamicIndexing;
		bool				m_shaderStorageBufferArrayDynamicIndexing;
		bool				m_shaderStorageImageArrayDynamicIndexing;
		bool				m_shaderInputAttachmentArrayDynamicIndexing;
		bool				m_shaderUniformTexelBufferArrayDynamicIndexing;
		bool				m_shaderStorageTexelBufferArrayDynamicIndexing;
		bool				m_shaderUniformBufferArrayNonUniformIndexing;
		bool				m_shaderSampledImageArrayNonUniformIndexing;
		bool				m_shaderStorageBufferArrayNonUniformIndexing;
		bool				m_shaderStorageImageArrayNonUniformIndexing;
		bool				m_shaderInputAttachmentArrayNonUniformIndexing;
		bool				m_shaderUniformTexelBufferArrayNonUniformIndexing;
		bool				m_shaderStorageTexelBufferArrayNonUniformIndexing;
		bool				m_shaderInt16;
		bool				m_shaderFloat64;
		bool				m_shaderStorageImageReadWithoutFormat;
		bool				m_shaderStorageImageExtendedFormats;
		bool				m_shaderStorageImageMultisample;
		bool				m_imageFormatList;
		bool				m_shaderClipDistance;
		bool				m_shaderCullDistance;
		bool				m_vertexPipelineStoresAndAtomics;
		bool				m_sampleRateShading;
		bool				m_imageCubeArray;
		bool				m_textureCompressionBC;
		bool				m_drawIndirectFirstInstance;
		bool				m_scalarBlockLayout;
		bool				m_runtimeDescriptorArray;
		bool				m_descriptorBindingPartiallyBound;
		bool				m_shaderOutputViewportIndex;
		bool				m_shaderOutputLayer;
		bool				m_subgroupBroadcastDynamicId;
		bool				m_shaderBufferFloat32Atomics;
		bool				m_shaderBufferFloat32AtomicAdd;
		bool				m_shaderBufferFloat64Atomics;
		bool				m_shaderBufferFloat64AtomicAdd;
		bool				m_shaderSharedFloat32Atomics;
		bool				m_shaderSharedFloat32AtomicAdd;
		bool				m_shaderSharedFloat64Atomics;
		bool				m_shaderSharedFloat64AtomicAdd;
		bool				m_shaderImageFloat32Atomics;
		bool				m_shaderImageFloat32AtomicAdd;
		bool				m_sparseImageFloat32Atomics;
		bool				m_sparseImageFloat32AtomicAdd;
		bool				m_shaderBufferFloat16Atomics;
		bool				m_shaderBufferFloat16AtomicAdd;
		bool				m_shaderBufferFloat16AtomicMinMax;
		bool				m_shaderBufferFloat32AtomicMinMax;
		bool				m_shaderBufferFloat64AtomicMinMax;
		bool				m_shaderSharedFloat16Atomics;
		bool				m_shaderSharedFloat16AtomicAdd;
		bool				m_shaderSharedFloat16AtomicMinMax;
		bool				m_shaderSharedFloat32AtomicMinMax;
		bool				m_shaderSharedFloat64AtomicMinMax;
		bool				m_shaderImageFloat32AtomicMinMax;
		bool				m_sparseImageFloat32AtomicMinMax;
		bool				m_shaderSubgroupClock;
		bool				m_shaderDeviceClock;
		bool				m_hlslFunctionality1;
		bool				m_deferredHostOperations;
		bool				m_pipelineLibrary;
		bool				m_spirv14;
		bool				m_shaderFloatControls;
		bool				m_accelerationStructure;
		bool				m_accelerationStructureCaptureReplay;
		bool				m_accelerationStructureIndirectBuild;
		bool				m_accelerationStructureHostCommands;
		bool				m_rayTracingPipeline;
		bool				m_rayTracingPipelineShaderGroupHandleCaptureReplay;
		bool				m_rayTracingPipelineShaderGroupHandleCaptureReplayMixed;
		bool				m_rayTracingPipelineTraceRaysIndirect;
		bool				m_rayTraversalPrimitiveCulling;
		bool				m_rayQuery;
		bool				m_rayTracingMaintenance1;
		bool				m_rayTracingPipelineTraceRaysIndirect2;
		bool				m_rayTracingPositionFetch;
		bool				m_micromap;
		bool				m_micromapKhr;
		bool				m_micromapCaptureReplay;
		bool				m_micromapHostCommands;
		bool				m_indexTypeUint8;
		bool				m_rayTracingInvocationReorder;
		bool				m_pipelineLibraryGroupHandles;
		bool				m_cooperativeMatrix;
		bool				m_cooperativeMatrixRobustBufferAccess;
		bool				m_shaderBFloat16Type;
		bool				m_shaderBFloat16DotProduct;
		bool				m_shaderBFloat16CooperativeMatrix;
		bool				m_shaderFloat8;
		bool				m_shaderFloat8CooperativeMatrix;
		bool				m_workgroupMemoryExplicitLayout;
		bool				m_workgroupMemoryExplicitLayoutScalarBlockLayout;
		bool				m_workgroupMemoryExplicitLayout8BitAccess;
		bool				m_workgroupMemoryExplicitLayout16BitAccess;
		bool				m_longVector;
		uint32_t			m_maxLongVectorComponents;
		bool				m_cooperativeMatrixProperties2;
		bool				m_cooperativeMatrixReductions;
		bool				m_cooperativeMatrixConversions;
		bool				m_cooperativeMatrixPerElementOperations;
		bool				m_cooperativeMatrixGetCoordinate;
		uint32_t			m_subgroupSize;
		VkShaderStageFlags	m_subgroupStages;
		VkSubgroupFeatureFlags m_subgroupOperations;
		bool				m_subgroupQuadOperationsInAllStages;
		bool				m_meshShader;
		bool				m_fragmentShadingRate;
		bool				m_conditionalRendering;
		bool				m_transformFeedback;
		bool				m_geometryStreams;
		bool				m_sampleLocations;
		bool				m_variableSampleLocations;
		float				m_sampleLocationRange[2];
		uint32_t			m_sampleLocationSubPixelBits;
		VkSampleCountFlags	m_sampleLocationCounts;
		VkExtent2D			m_sampleLocationGrids[5];
		bool				m_conservativeRaster;
		bool				m_emulateD24S8;
		bool				m_ownsInstance;
		bool				m_sparseBinding;
		bool				m_sparseResidencyBuffer;
		bool				m_sparseResidencyImage2D;
		bool				m_sparseResidencyImage3D;
		bool				m_sparseResidency2Samples;
		bool				m_sparseResidency4Samples;
		bool				m_sparseResidency8Samples;
		bool				m_sparseResidency16Samples;
		bool				m_sparseResidencyAliased;

		VkPhysicalDeviceDescriptorHeapPropertiesEXT				m_heapProps;
		VkPhysicalDeviceDeviceGeneratedCommandsPropertiesEXT	m_dgcProps;

		PFN_vkCmdBeginRendering			m_beginRendering;
		PFN_vkCmdEndRendering			m_endRendering;
		PFN_vkCmdSetPrimitiveTopology	m_setPrimitiveTopology;
		PFN_vkCmdBindVertexBuffers2		m_bindVertexBuffers2;
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtPipelineProps;
		ExtProcs						m_ext;

		struct QueueFamilyPool
		{
			uint32_t Family;
			uint32_t Count;
			mutable uint32_t Used[4];
		};

		QueueFamilyPool		m_queuePools[3];
		uint32_t			m_poolCount;
		uint32_t			m_typePool[3];
		uint32_t			m_sharingFamilies[3];
		uint32_t			m_sharingCount;
		mutable VkSemaphore	m_sparseTimeline;
		mutable uint64_t	m_sparseValue;
		mutable std::mutex	m_queueMutex;
	};

	class Fence_VK :
		public virtual Fence
	{
	public:
		Fence_VK();
		virtual ~Fence_VK();

		bool Create(Device* pDevice, uint64_t initialValue,
			FenceFlag flags, const wchar_t* name = nullptr);
		bool SetEventOnCompletion(uint64_t value, void* hEvent);
		bool Signal(uint64_t value);

		uint64_t GetCompletedValue() const;

		void Create(uint64_t nativeHandle, Device* pDevice);

		uint64_t GetNativeHandle() const;

		VkSemaphore GetSemaphore() const;

	protected:
		Device_VK*	m_pDevice;
		VkSemaphore	m_semaphore;
	};

	class CommandLayout_VK :
		public virtual CommandLayout
	{
	public:
		CommandLayout_VK();
		virtual ~CommandLayout_VK();

		bool Create(Device* pDevice, uint32_t byteStride, uint32_t numArguments,
			const IndirectArgument* pArguments, const PipelineLayout& pipelineLayout = nullptr,
			uint32_t nodeMask = 0, const wchar_t* name = nullptr);

		void Create(uint64_t nativeHandle, Device* pDevice);

		uint64_t GetNativeHandle() const;
		void* GetBackend() override;

		VkIndirectCommandsLayoutEXT GetLayout() const;
		VkShaderStageFlags GetShaderStages() const;
		uint32_t GetByteStride() const;
		bool IsDispatch() const;
		bool IsTraceRays() const;
		bool IsTraceRaysOnly() const;

	protected:
		Device_VK*					m_pDevice;
		VkIndirectCommandsLayoutEXT	m_layout;
		VkShaderStageFlags			m_stages;
		uint32_t					m_stride;
		bool						m_isDispatch;
		bool						m_traceRays;
		bool						m_traceRaysOnly;

		std::vector<VkIndirectCommandsLayoutTokenEXT>		m_tokens;
		std::vector<VkIndirectCommandsVertexBufferTokenEXT>	m_vertexTokens;
		std::vector<VkIndirectCommandsIndexBufferTokenEXT>	m_indexTokens;
		std::vector<VkIndirectCommandsPushConstantTokenEXT>	m_pushTokens;
	};

	class SwapChain_VK :
		public virtual SwapChain
	{
	public:
		SwapChain_VK();
		virtual ~SwapChain_VK();

		bool Create(void* pFactory, void* hWnd, void* pDevice, uint8_t bufferCount,
			uint32_t width, uint32_t height, Format format, SwapChainFlag flags = SwapChainFlag::NONE,
			bool windowed = true);
		bool Present(uint8_t syncInterval = 0, PresentFlag flags = PresentFlag::NONE);
		bool PresentEx(uint8_t syncInterval, PresentFlag flags, uint32_t dirtyRectsCount,
			const RectRange* pDirtyRects, const RectRange* pScrollRect = nullptr,
			const Point* pScrollOffset = nullptr);
		bool GetBuffer(uint8_t buffer, Resource* pResource) const;

		uint32_t ResizeBuffers(uint8_t bufferCount, uint32_t width, uint32_t height,
			Format format, SwapChainFlag flags = SwapChainFlag::NONE);

		uint8_t GetCurrentBackBufferIndex() const;

		void Create(uint64_t nativeHandle);

		uint64_t GetNativeHandle() const;
		void* GetBackend() override;

		VkImage GetImage(uint8_t buffer) const;
		VkFormat GetVkFormat() const;
		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

	protected:
		friend bool FlushQueuePendingPresent(VkQueue queue);

		bool createSwapchain(uint8_t bufferCount, uint32_t width, uint32_t height, Format format, VkSwapchainKHR old);
		bool createUserImages();
		void destroyUserImages();
		bool createBlitResources();
		void destroyBlitResources();
		bool acquire() const;
		bool flushPresent(uint32_t userIndex);

		Device_VK*			m_pDevice;
		VkQueue				m_queue;
		uint32_t			m_family;
		VkSurfaceKHR		m_surface;
		VkSwapchainKHR		m_swapChain;

		VkFormat			m_format;
		uint32_t			m_width;
		uint32_t			m_height;
		uint8_t				m_bufferCount;
		mutable uint32_t	m_imageIndex;
		mutable uint32_t	m_userIndex;
		bool				m_allowTearing;
		mutable bool		m_acquired;
		mutable VkSemaphore	m_lastAcquireSemaphore;
		mutable VkFence		m_lastAcquireFence;

		struct AcquireSlot
		{
			VkSemaphore Semaphore;
			VkFence Fence;
			bool Submitted;
		};

		mutable std::vector<AcquireSlot> m_acquireSlots;
		mutable uint32_t	m_acquireSlot;
		std::vector<VkSemaphore> m_releaseSemaphores;

		std::vector<VkImage> m_images;
		std::vector<VkImage> m_userImages;
		std::vector<VkDeviceMemory> m_userMemory;

		VkCommandPool		m_blitPool;
		std::vector<VkCommandBuffer> m_blitBuffers;
		uint32_t			m_blitSlot;
	};

	void GetVKBlobData(const Blob& blob, const void** ppData, size_t* pSize);
	bool FindVKRootSignaturePayload(const void* pData, size_t size, const wchar_t* name,
		const void** ppPayload, size_t* pPayloadSize);
	Blob GetVKPipelineCache(Pipeline pipeline);

	std::string WideToUtf8(const wchar_t* name);

	// Virtual inheritance blocks static_cast from Device*/CommandLayout* to *_VK*.
	// GetBackend() returns the concrete subobject; AsVK recovers it without RTTI.
	inline Device_VK* AsVK(Device* p)
	{
		assert(!p || p->GetBackend());
		return static_cast<Device_VK*>(p ? p->GetBackend() : nullptr);
	}

	inline const Device_VK* AsVK(const Device* p)
	{
		assert(!p || const_cast<Device*>(p)->GetBackend());
		return static_cast<const Device_VK*>(p ? const_cast<Device*>(p)->GetBackend() : nullptr);
	}

	inline CommandLayout_VK* AsVK(CommandLayout* p)
	{
		assert(!p || p->GetBackend());
		return static_cast<CommandLayout_VK*>(p ? p->GetBackend() : nullptr);
	}

	inline const CommandLayout_VK* AsVK(const CommandLayout* p)
	{
		assert(!p || const_cast<CommandLayout*>(p)->GetBackend());
		return static_cast<const CommandLayout_VK*>(p ? const_cast<CommandLayout*>(p)->GetBackend() : nullptr);
	}

	inline SwapChain_VK* AsVK(SwapChain* p)
	{
		assert(!p || p->GetBackend());
		return static_cast<SwapChain_VK*>(p ? p->GetBackend() : nullptr);
	}

	inline const SwapChain_VK* AsVK(const SwapChain* p)
	{
		assert(!p || const_cast<SwapChain*>(p)->GetBackend());
		return static_cast<const SwapChain_VK*>(p ? const_cast<SwapChain*>(p)->GetBackend() : nullptr);
	}
}
