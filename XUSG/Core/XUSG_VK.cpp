//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_VK.h"
#include "XUSGResource_VK.h"
#include "XUSGEnum_VK.h"

using namespace std;
using namespace XUSG;

namespace
{
	mutex g_queueMutex;
	unordered_map<VkQueue, QueueNative> g_queues;
	unordered_map<VkQueue, uint32_t> g_queueRefs;
	struct AcquireWait
	{
		VkSemaphore Wait;
		VkFence Fence;
		VkSemaphore PresentSignal;
	};

	unordered_map<VkQueue, AcquireWait> g_acquireWaits;
	unordered_map<VkQueue, QueuePendingPresent> g_pendingPresents;

	bool SubmitQueueInfo(bool synchronization2, VkQueue queue, const VkSubmitInfo2& info, VkFence fence,
		VkSemaphore* pWaitSemaphores, VkPipelineStageFlags* pWaitStages, uint64_t* pWaitValues,
		VkCommandBuffer* pCommandBuffers, VkSemaphore* pSignalSemaphores, uint64_t* pSignalValues)
	{
		if (synchronization2)
		{
			XUSG_VK_CHECK(vkQueueSubmit2(queue, 1, &info, fence), cerr, false);

			return true;
		}

		for (auto i = 0u; i < info.waitSemaphoreInfoCount; ++i)
		{
			pWaitSemaphores[i] = info.pWaitSemaphoreInfos[i].semaphore;
			const auto stage = info.pWaitSemaphoreInfos[i].stageMask;
			pWaitStages[i] = stage ? static_cast<VkPipelineStageFlags>(stage) : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			pWaitValues[i] = info.pWaitSemaphoreInfos[i].value;
		}
		for (auto i = 0u; i < info.commandBufferInfoCount; ++i)
			pCommandBuffers[i] = info.pCommandBufferInfos[i].commandBuffer;
		for (auto i = 0u; i < info.signalSemaphoreInfoCount; ++i)
		{
			pSignalSemaphores[i] = info.pSignalSemaphoreInfos[i].semaphore;
			pSignalValues[i] = info.pSignalSemaphoreInfos[i].value;
		}

		VkTimelineSemaphoreSubmitInfo timeline = {};
		timeline.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timeline.waitSemaphoreValueCount = info.waitSemaphoreInfoCount;
		timeline.pWaitSemaphoreValues = info.waitSemaphoreInfoCount ? pWaitValues : nullptr;
		timeline.signalSemaphoreValueCount = info.signalSemaphoreInfoCount;
		timeline.pSignalSemaphoreValues = info.signalSemaphoreInfoCount ? pSignalValues : nullptr;

		VkSubmitInfo submit = {};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		if (info.waitSemaphoreInfoCount || info.signalSemaphoreInfoCount) submit.pNext = &timeline;
		submit.waitSemaphoreCount = info.waitSemaphoreInfoCount;
		submit.pWaitSemaphores = info.waitSemaphoreInfoCount ? pWaitSemaphores : nullptr;
		submit.pWaitDstStageMask = info.waitSemaphoreInfoCount ? pWaitStages : nullptr;
		submit.commandBufferCount = info.commandBufferInfoCount;
		submit.pCommandBuffers = info.commandBufferInfoCount ? pCommandBuffers : nullptr;
		submit.signalSemaphoreCount = info.signalSemaphoreInfoCount;
		submit.pSignalSemaphores = info.signalSemaphoreInfoCount ? pSignalSemaphores : nullptr;
		XUSG_VK_CHECK(vkQueueSubmit(queue, 1, &submit, fence), cerr, false);

		return true;
	}

	bool EnvContains(const char* name, const char* token)
	{
		char* value = nullptr;
		size_t length = 0;
		if (_dupenv_s(&value, &length, name) || !value) return false;

		auto found = false;
		for (auto p = value; *p; )
		{
			while (*p == ',' || *p == ' ' || *p == ';') ++p;
			const auto start = p;
			while (*p && *p != ',' && *p != ' ' && *p != ';') ++p;
			const auto count = static_cast<size_t>(p - start);
			if (count == strlen(token) && strncmp(start, token, count) == 0) found = true;
		}
		free(value);

		return found;
	}

	bool SingleQueueRequested()
	{
		char* value = nullptr;
		size_t length = 0;
		if (_dupenv_s(&value, &length, "XUSG_SINGLE_QUEUE") == 0 && value)
		{
			const auto enabled = value[0] && value[0] != '0';
			free(value);
			if (enabled) return true;
		}

		return EnvContains("VKD3D_CONFIG", "single_queue");
	}

	uint32_t FindQueueFamily(const vector<VkQueueFamilyProperties>& families, VkQueueFlags mask, VkQueueFlags flags)
	{
		for (auto i = 0u; i < families.size(); ++i)
		{
			const auto& family = families[i];
			if ((family.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
				(family.minImageTransferGranularity.width == 0 ||
					family.minImageTransferGranularity.height == 0 ||
					family.minImageTransferGranularity.depth == 0))
				continue;
			if ((family.queueFlags & mask) == flags) return i;
		}

		return UINT32_MAX;
	}

	uint32_t QueueTypeIndex(CommandListType type)
	{
		if (type == CommandListType::COMPUTE) return 1;
		if (type == CommandListType::COPY) return 2;

		return 0;
	}

	bool HasExtension(const vector<VkExtensionProperties>& props, const char* name)
	{
		for (const auto& prop : props)
			if (strcmp(prop.extensionName, name) == 0) return true;

		return false;
	}

#if defined(_DEBUG)
	bool HasLayer(const vector<VkLayerProperties>& props, const char* name)
	{
		for (const auto& prop : props)
			if (strcmp(prop.layerName, name) == 0) return true;

		return false;
	}
#endif
}

void XUSG::RegisterQueue(const QueueNative& queue)
{
	lock_guard<mutex> lock(g_queueMutex);
	g_queues[queue.Queue] = queue;
	++g_queueRefs[queue.Queue];
}

void XUSG::UnregisterQueue(VkQueue queue)
{
	lock_guard<mutex> lock(g_queueMutex);
	const auto it = g_queueRefs.find(queue);
	if (it == g_queueRefs.cend()) return;

	if (it->second > 1) --it->second;
	else
	{
		g_queueRefs.erase(it);
		g_queues.erase(queue);
	}
}

bool XUSG::FindQueue(VkQueue queue, QueueNative& native)
{
	lock_guard<mutex> lock(g_queueMutex);
	const auto it = g_queues.find(queue);
	if (it == g_queues.cend()) return false;

	native = it->second;

	return true;
}

void XUSG::SetQueueAcquireWait(VkQueue queue, VkSemaphore wait, VkFence fence, VkSemaphore presentSignal)
{
	lock_guard<mutex> lock(g_queueMutex);
	g_acquireWaits[queue] = { wait, fence, presentSignal };
}

bool XUSG::TakeQueueAcquireWait(VkQueue queue, VkSemaphore& wait, VkFence& fence, VkSemaphore& presentSignal)
{
	lock_guard<mutex> lock(g_queueMutex);
	const auto it = g_acquireWaits.find(queue);
	if (it == g_acquireWaits.cend() || !it->second.Wait) return false;

	wait = it->second.Wait;
	fence = it->second.Fence;
	presentSignal = it->second.PresentSignal;
	it->second = {};

	return true;
}

void XUSG::ClearQueueAcquireWait(VkQueue queue)
{
	lock_guard<mutex> lock(g_queueMutex);
	g_acquireWaits.erase(queue);
}

void XUSG::SetQueuePendingPresent(VkQueue queue, const QueuePendingPresent& present)
{
	lock_guard<mutex> lock(g_queueMutex);
	g_pendingPresents[queue] = present;
}

bool XUSG::TakeQueuePendingPresent(VkQueue queue, QueuePendingPresent& present)
{
	lock_guard<mutex> lock(g_queueMutex);
	const auto it = g_pendingPresents.find(queue);
	if (it == g_pendingPresents.cend() || !it->second.SwapChain) return false;

	present = it->second;
	g_pendingPresents.erase(it);

	return true;
}

bool XUSG::HasQueuePendingPresent(VkQueue queue)
{
	lock_guard<mutex> lock(g_queueMutex);
	const auto it = g_pendingPresents.find(queue);

	return it != g_pendingPresents.cend() && it->second.SwapChain != nullptr;
}

bool XUSG::FlushQueuePendingPresent(VkQueue queue)
{
	QueuePendingPresent present = {};
	if (!TakeQueuePendingPresent(queue, present)) return true;
	if (!present.SwapChain) return false;

	return present.SwapChain->flushPresent(present.UserIndex);
}

PipelineLayoutObject::PipelineLayoutObject() :
	StageMappingInfos(),
	PushDataSize(0)
{
	for (auto& info : StageMappingInfos)
	{
		info = {};
		info.sType = VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT;
	}
}

static void AppendUtf8(string& utf8, uint32_t code)
{
	if (code < 0x80)
		utf8.push_back(static_cast<char>(code));
	else if (code < 0x800)
	{
		utf8.push_back(static_cast<char>(0xC0 | (code >> 6)));
		utf8.push_back(static_cast<char>(0x80 | (code & 0x3F)));
	}
	else if (code < 0x10000)
	{
		utf8.push_back(static_cast<char>(0xE0 | (code >> 12)));
		utf8.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
		utf8.push_back(static_cast<char>(0x80 | (code & 0x3F)));
	}
	else
	{
		utf8.push_back(static_cast<char>(0xF0 | (code >> 18)));
		utf8.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
		utf8.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
		utf8.push_back(static_cast<char>(0x80 | (code & 0x3F)));
	}
}

string XUSG::WideToUtf8(const wchar_t* name)
{
	if (!name) return {};

	string utf8;
	while (*name)
	{
		uint32_t code = static_cast<uint32_t>(*name++);
		if constexpr (sizeof(wchar_t) == 2)
		{
			if (code >= 0xD800 && code <= 0xDBFF && *name)
			{
				const auto low = static_cast<uint32_t>(*name);
				if (low >= 0xDC00 && low <= 0xDFFF)
				{
					++name;
					code = 0x10000 + (((code - 0xD800) << 10) | (low - 0xDC00));
				}
			}
		}
		AppendUtf8(utf8, code);
	}

	return utf8;
}

//--------------------------------------------------------------------------------------

Device_VK::Device_VK() :
	m_instance(VK_NULL_HANDLE),
	m_physicalDevice(VK_NULL_HANDLE),
	m_device(VK_NULL_HANDLE),
	m_apiVersion(0),
	m_graphicsFamily(0),
	m_resourceStride(0),
	m_samplerStride(0),
	m_removedReason(0),
	m_debugUtils(false),
	m_customBorderColor(false),
	m_vertexAttributeDivisor(false),
	m_shaderUntypedPointers(false),
	m_nullDescriptor(false),
	m_deviceGeneratedCommands(false),
	m_maintenance5(false),
	m_synchronization2(false),
	m_depthStencilResolve(false),
	m_independentResolveNone(false),
	m_independentResolve(false),
	m_depthResolveModes(0),
	m_stencilResolveModes(0),
	m_shaderInt64(false),
	m_shaderInt8(false),
	m_storageBuffer8BitAccess(false),
	m_uniformAndStorageBuffer8BitAccess(false),
	m_storagePushConstant8(false),
	m_storageBuffer16BitAccess(false),
	m_uniformAndStorageBuffer16BitAccess(false),
	m_storagePushConstant16(false),
	m_storageInputOutput16(false),
	m_shaderDrawParameters(false),
	m_shaderSubgroupExtendedTypes(false),
	m_shaderBufferInt64Atomics(false),
	m_shaderSharedInt64Atomics(false),
	m_shaderImageInt64Atomics(false),
	m_sparseImageInt64Atomics(false),
	m_subgroupSizeControl(false),
	m_computeFullSubgroups(false),
	m_shaderIntegerDotProduct(false),
	m_shaderDemoteToHelperInvocation(false),
	m_shaderSubgroupRotate(false),
	m_shaderSubgroupRotateClustered(false),
	m_shaderFloatControls2(false),
	m_shaderSubgroupPartitioned(false),
	m_shaderQuadControl(false),
	m_shaderSubgroupUniformControlFlow(false),
	m_shaderMaximalReconvergence(false),
	m_computeDerivativeGroupQuads(false),
	m_computeDerivativeGroupLinear(false),
	m_fragmentShaderBarycentric(false),
	m_shaderSubgroupBallot(false),
	m_shaderSubgroupVote(false),
	m_shaderUniformBufferArrayDynamicIndexing(false),
	m_shaderSampledImageArrayDynamicIndexing(false),
	m_shaderStorageBufferArrayDynamicIndexing(false),
	m_shaderStorageImageArrayDynamicIndexing(false),
	m_shaderInputAttachmentArrayDynamicIndexing(false),
	m_shaderUniformTexelBufferArrayDynamicIndexing(false),
	m_shaderStorageTexelBufferArrayDynamicIndexing(false),
	m_shaderUniformBufferArrayNonUniformIndexing(false),
	m_shaderSampledImageArrayNonUniformIndexing(false),
	m_shaderStorageBufferArrayNonUniformIndexing(false),
	m_shaderStorageImageArrayNonUniformIndexing(false),
	m_shaderInputAttachmentArrayNonUniformIndexing(false),
	m_shaderUniformTexelBufferArrayNonUniformIndexing(false),
	m_shaderStorageTexelBufferArrayNonUniformIndexing(false),
	m_shaderInt16(false),
	m_shaderFloat64(false),
	m_shaderStorageImageReadWithoutFormat(false),
	m_shaderStorageImageExtendedFormats(false),
	m_shaderStorageImageMultisample(false),
	m_imageFormatList(false),
	m_shaderClipDistance(false),
	m_shaderCullDistance(false),
	m_vertexPipelineStoresAndAtomics(false),
	m_sampleRateShading(false),
	m_imageCubeArray(false),
	m_textureCompressionBC(false),
	m_drawIndirectFirstInstance(false),
	m_scalarBlockLayout(false),
	m_runtimeDescriptorArray(false),
	m_descriptorBindingPartiallyBound(false),
	m_shaderOutputViewportIndex(false),
	m_shaderOutputLayer(false),
	m_subgroupBroadcastDynamicId(false),
	m_shaderBufferFloat32Atomics(false),
	m_shaderBufferFloat32AtomicAdd(false),
	m_shaderBufferFloat64Atomics(false),
	m_shaderBufferFloat64AtomicAdd(false),
	m_shaderSharedFloat32Atomics(false),
	m_shaderSharedFloat32AtomicAdd(false),
	m_shaderSharedFloat64Atomics(false),
	m_shaderSharedFloat64AtomicAdd(false),
	m_shaderImageFloat32Atomics(false),
	m_shaderImageFloat32AtomicAdd(false),
	m_sparseImageFloat32Atomics(false),
	m_sparseImageFloat32AtomicAdd(false),
	m_shaderBufferFloat16Atomics(false),
	m_shaderBufferFloat16AtomicAdd(false),
	m_shaderBufferFloat16AtomicMinMax(false),
	m_shaderBufferFloat32AtomicMinMax(false),
	m_shaderBufferFloat64AtomicMinMax(false),
	m_shaderSharedFloat16Atomics(false),
	m_shaderSharedFloat16AtomicAdd(false),
	m_shaderSharedFloat16AtomicMinMax(false),
	m_shaderSharedFloat32AtomicMinMax(false),
	m_shaderSharedFloat64AtomicMinMax(false),
	m_shaderImageFloat32AtomicMinMax(false),
	m_sparseImageFloat32AtomicMinMax(false),
	m_shaderSubgroupClock(false),
	m_shaderDeviceClock(false),
	m_hlslFunctionality1(false),
	m_deferredHostOperations(false),
	m_pipelineLibrary(false),
	m_spirv14(false),
	m_shaderFloatControls(false),
	m_accelerationStructure(false),
	m_accelerationStructureCaptureReplay(false),
	m_accelerationStructureIndirectBuild(false),
	m_accelerationStructureHostCommands(false),
	m_rayTracingPipeline(false),
	m_rayTracingPipelineShaderGroupHandleCaptureReplay(false),
	m_rayTracingPipelineShaderGroupHandleCaptureReplayMixed(false),
	m_rayTracingPipelineTraceRaysIndirect(false),
	m_rayTraversalPrimitiveCulling(false),
	m_rayQuery(false),
	m_rayTracingMaintenance1(false),
	m_rayTracingPipelineTraceRaysIndirect2(false),
	m_rayTracingPositionFetch(false),
	m_micromap(false),
	m_micromapKhr(false),
	m_micromapCaptureReplay(false),
	m_micromapHostCommands(false),
	m_indexTypeUint8(false),
	m_rayTracingInvocationReorder(false),
	m_pipelineLibraryGroupHandles(false),
	m_cooperativeMatrix(false),
	m_cooperativeMatrixRobustBufferAccess(false),
	m_shaderBFloat16Type(false),
	m_shaderBFloat16DotProduct(false),
	m_shaderBFloat16CooperativeMatrix(false),
	m_shaderFloat8(false),
	m_shaderFloat8CooperativeMatrix(false),
	m_workgroupMemoryExplicitLayout(false),
	m_workgroupMemoryExplicitLayoutScalarBlockLayout(false),
	m_workgroupMemoryExplicitLayout8BitAccess(false),
	m_workgroupMemoryExplicitLayout16BitAccess(false),
	m_longVector(false),
	m_maxLongVectorComponents(0),
	m_cooperativeMatrixProperties2(false),
	m_cooperativeMatrixReductions(false),
	m_cooperativeMatrixConversions(false),
	m_cooperativeMatrixPerElementOperations(false),
	m_cooperativeMatrixGetCoordinate(false),
	m_subgroupSize(0),
	m_subgroupStages(0),
	m_subgroupOperations(0),
	m_subgroupQuadOperationsInAllStages(false),
	m_meshShader(false),
	m_fragmentShadingRate(false),
	m_conditionalRendering(false),
	m_transformFeedback(false),
	m_geometryStreams(false),
	m_sampleLocations(false),
	m_variableSampleLocations(false),
	m_sampleLocationRange{},
	m_sampleLocationSubPixelBits(0),
	m_sampleLocationCounts(0),
	m_sampleLocationGrids{},
	m_conservativeRaster(false),
	m_emulateD24S8(false),
	m_ownsInstance(false),
	m_sparseBinding(false),
	m_sparseResidencyBuffer(false),
	m_sparseResidencyImage2D(false),
	m_sparseResidencyImage3D(false),
	m_sparseResidency2Samples(false),
	m_sparseResidency4Samples(false),
	m_sparseResidency8Samples(false),
	m_sparseResidency16Samples(false),
	m_sparseResidencyAliased(false),
	m_heapProps{},
	m_dgcProps{},
	m_beginRendering(nullptr),
	m_endRendering(nullptr),
	m_setPrimitiveTopology(nullptr),
	m_bindVertexBuffers2(nullptr),
	m_ext{},
	m_queuePools{},
	m_poolCount(0),
	m_typePool{},
	m_sharingFamilies{},
	m_sharingCount(0),
	m_sparseTimeline(VK_NULL_HANDLE),
	m_sparseValue(0)
{
	m_rtPipelineProps = {};
	m_rtPipelineProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	m_rtPipelineProps.shaderGroupHandleSize = 32;
	m_rtPipelineProps.shaderGroupHandleAlignment = 32;
	m_rtPipelineProps.shaderGroupBaseAlignment = 64;
	m_rtPipelineProps.maxRayRecursionDepth = 1;
}

Device_VK::~Device_VK()
{
	if (m_sparseTimeline != VK_NULL_HANDLE) vkDestroySemaphore(m_device, m_sparseTimeline, nullptr);
	if (m_device != VK_NULL_HANDLE) vkDestroyDevice(m_device, nullptr);
	if (m_ownsInstance && m_instance != VK_NULL_HANDLE) vkDestroyInstance(m_instance, nullptr);
}

bool Device_VK::GetCommandQueue(CommandQueue* pCommandQueue, CommandListType type,
	CommandQueueFlag flags, int32_t priority, uint32_t nodeMask, const wchar_t* name)
{
	return pCommandQueue->Create(this, type, flags, priority, nodeMask, name);
}

bool Device_VK::GetCommandAllocator(CommandAllocator* pCommandAllocator,
	CommandListType type, const wchar_t* name)
{
	return pCommandAllocator->Create(this, type, name);
}

bool Device_VK::GetCommandList(CommandList* pCommandList, uint32_t nodeMask, CommandListType type,
	CommandAllocator* pCommandAllocator, const Pipeline& pipeline, const wchar_t* name)
{
	return pCommandList->Create(this, nodeMask, type, pCommandAllocator, pipeline, name);
}

bool Device_VK::GetFence(Fence* pFence, uint64_t initialValue, FenceFlag flags, const wchar_t* name)
{
	return pFence->Create(this, initialValue, flags, name);
}

uint32_t Device_VK::Create(void* pAdapter, uint32_t minFeatureLevel, const wchar_t* name)
{
	// minFeatureLevel is a D3D feature level on the DX12 path. Vulkan requests 1.3 when the
	// loader supports it, and falls back through 1.2 and 1.1 to 1.0. The feature-level
	// argument is not a Vulkan version.
	(void)minFeatureLevel;

	XUSG_N_RETURN(createInstance(), static_cast<uint32_t>(VK_ERROR_INITIALIZATION_FAILED));
	XUSG_N_RETURN(pickPhysicalDevice(static_cast<VkPhysicalDevice>(pAdapter)), static_cast<uint32_t>(VK_ERROR_INITIALIZATION_FAILED));
	XUSG_N_RETURN(createDevice(), static_cast<uint32_t>(VK_ERROR_INITIALIZATION_FAILED));

	if (name) SetDebugName(VK_OBJECT_TYPE_DEVICE, reinterpret_cast<uintptr_t>(m_device), name);

	return VK_SUCCESS;
}

uint32_t Device_VK::GetDeviceRemovedReason() const
{
	return m_removedReason;
}

void Device_VK::Create(void* pNativeHandle)
{
	m_device = static_cast<VkDevice>(pNativeHandle);
}

void* Device_VK::GetNativeHandle()
{
	return m_device;
}

void* Device_VK::GetAdapterNativeHandle()
{
	return m_physicalDevice;
}

void* Device_VK::GetBackend()
{
	return this;
}

VkInstance Device_VK::GetInstance() const
{
	return m_instance;
}

VkPhysicalDevice Device_VK::GetPhysicalDevice() const
{
	return m_physicalDevice;
}

VkDevice Device_VK::GetDevice() const
{
	return m_device;
}

uint32_t Device_VK::GetApiVersion() const
{
	return m_apiVersion;
}

uint32_t Device_VK::GetGraphicsFamily() const
{
	return m_graphicsFamily;
}

uint32_t Device_VK::GetQueueFamily(CommandListType type) const
{
	return m_queuePools[m_typePool[QueueTypeIndex(type)]].Family;
}

void Device_VK::GetSharingFamilies(uint32_t& count, const uint32_t*& pFamilies) const
{
	count = m_sharingCount > 1 ? m_sharingCount : 0;
	pFamilies = m_sharingFamilies;
}

bool Device_VK::AcquireQueue(CommandListType type, uint32_t& family, uint32_t& queueIndex) const
{
	auto& pool = m_queuePools[m_typePool[QueueTypeIndex(type)]];
	assert(pool.Count);
	if (!pool.Count) return false;

	lock_guard<mutex> lock(m_queueMutex);
	auto index = 0u;
	for (auto i = 1u; i < pool.Count; ++i)
		if (pool.Used[i] < pool.Used[index]) index = i;
	++pool.Used[index];
	family = pool.Family;
	queueIndex = index;

	return true;
}

void Device_VK::ReleaseQueue(uint32_t family, uint32_t queueIndex) const
{
	lock_guard<mutex> lock(m_queueMutex);
	for (auto i = 0u; i < m_poolCount; ++i)
	{
		if (m_queuePools[i].Family != family) continue;
		assert(queueIndex < m_queuePools[i].Count);
		assert(m_queuePools[i].Used[queueIndex]);
		if (queueIndex < m_queuePools[i].Count && m_queuePools[i].Used[queueIndex])
			--m_queuePools[i].Used[queueIndex];

		return;
	}
}

uint32_t Device_VK::GetResourceDescriptorStride() const
{
	return m_resourceStride;
}

uint32_t Device_VK::GetSamplerDescriptorStride() const
{
	return m_samplerStride;
}

const VkPhysicalDeviceDescriptorHeapPropertiesEXT& Device_VK::GetDescriptorHeapProperties() const
{
	return m_heapProps;
}

const VkPhysicalDeviceDeviceGeneratedCommandsPropertiesEXT& Device_VK::GetGeneratedCommandsProperties() const
{
	return m_dgcProps;
}

PFN_vkCmdBeginRendering Device_VK::GetBeginRendering() const
{
	return m_beginRendering;
}

PFN_vkCmdEndRendering Device_VK::GetEndRendering() const
{
	return m_endRendering;
}

PFN_vkCmdSetPrimitiveTopology Device_VK::GetSetPrimitiveTopology() const
{
	return m_setPrimitiveTopology;
}

PFN_vkCmdBindVertexBuffers2 Device_VK::GetBindVertexBuffers2() const
{
	return m_bindVertexBuffers2;
}

const Device_VK::ExtProcs& Device_VK::GetExtProcs() const
{
	return m_ext;
}

bool Device_VK::HasDebugUtils() const
{
	return m_debugUtils;
}

bool Device_VK::HasCustomBorderColor() const
{
	return m_customBorderColor;
}

bool Device_VK::HasVertexAttributeDivisor() const
{
	return m_vertexAttributeDivisor;
}

bool Device_VK::HasShaderUntypedPointers() const
{
	return m_shaderUntypedPointers;
}

bool Device_VK::HasNullDescriptor() const
{
	return m_nullDescriptor;
}

bool Device_VK::HasDeviceGeneratedCommands() const
{
	return m_deviceGeneratedCommands;
}

bool Device_VK::HasMaintenance5() const
{
	return m_maintenance5;
}

bool Device_VK::HasSynchronization2() const
{
	return m_synchronization2;
}

bool Device_VK::HasDepthStencilResolve() const
{
	return m_depthStencilResolve;
}

VkResolveModeFlags Device_VK::SupportedDepthResolveModes() const
{
	return m_depthResolveModes;
}

VkResolveModeFlags Device_VK::SupportedStencilResolveModes() const
{
	return m_stencilResolveModes;
}

bool Device_VK::IndependentDepthStencilResolveNone() const
{
	return m_independentResolveNone;
}

bool Device_VK::IndependentDepthStencilResolve() const
{
	return m_independentResolve;
}

bool Device_VK::HasShaderInt64() const
{
	return m_shaderInt64;
}

bool Device_VK::HasShaderInt8() const
{
	return m_shaderInt8;
}

bool Device_VK::HasStorageBuffer8BitAccess() const
{
	return m_storageBuffer8BitAccess;
}

bool Device_VK::HasUniformAndStorageBuffer8BitAccess() const
{
	return m_uniformAndStorageBuffer8BitAccess;
}

bool Device_VK::HasStoragePushConstant8() const
{
	return m_storagePushConstant8;
}

bool Device_VK::HasStorageBuffer16BitAccess() const
{
	return m_storageBuffer16BitAccess;
}

bool Device_VK::HasUniformAndStorageBuffer16BitAccess() const
{
	return m_uniformAndStorageBuffer16BitAccess;
}

bool Device_VK::HasStoragePushConstant16() const
{
	return m_storagePushConstant16;
}

bool Device_VK::HasStorageInputOutput16() const
{
	return m_storageInputOutput16;
}

bool Device_VK::HasShaderDrawParameters() const
{
	return m_shaderDrawParameters;
}

bool Device_VK::HasShaderSubgroupExtendedTypes() const
{
	return m_shaderSubgroupExtendedTypes;
}

bool Device_VK::HasShaderBufferInt64Atomics() const
{
	return m_shaderBufferInt64Atomics;
}

bool Device_VK::HasShaderSharedInt64Atomics() const
{
	return m_shaderSharedInt64Atomics;
}

bool Device_VK::HasShaderImageInt64Atomics() const
{
	return m_shaderImageInt64Atomics;
}

bool Device_VK::HasSparseImageInt64Atomics() const
{
	return m_sparseImageInt64Atomics;
}

bool Device_VK::HasSubgroupSizeControl() const
{
	return m_subgroupSizeControl;
}

bool Device_VK::HasComputeFullSubgroups() const
{
	return m_computeFullSubgroups;
}

bool Device_VK::HasShaderIntegerDotProduct() const
{
	return m_shaderIntegerDotProduct;
}

bool Device_VK::HasShaderDemoteToHelperInvocation() const
{
	return m_shaderDemoteToHelperInvocation;
}

bool Device_VK::HasShaderSubgroupRotate() const
{
	return m_shaderSubgroupRotate;
}

bool Device_VK::HasShaderSubgroupRotateClustered() const
{
	return m_shaderSubgroupRotateClustered;
}

bool Device_VK::HasShaderFloatControls2() const
{
	return m_shaderFloatControls2;
}

bool Device_VK::HasShaderSubgroupPartitioned() const
{
	return m_shaderSubgroupPartitioned;
}

bool Device_VK::HasShaderQuadControl() const
{
	return m_shaderQuadControl;
}

bool Device_VK::HasShaderSubgroupUniformControlFlow() const
{
	return m_shaderSubgroupUniformControlFlow;
}

bool Device_VK::HasShaderMaximalReconvergence() const
{
	return m_shaderMaximalReconvergence;
}

bool Device_VK::HasComputeDerivativeGroupQuads() const
{
	return m_computeDerivativeGroupQuads;
}

bool Device_VK::HasComputeDerivativeGroupLinear() const
{
	return m_computeDerivativeGroupLinear;
}

bool Device_VK::HasFragmentShaderBarycentric() const
{
	return m_fragmentShaderBarycentric;
}

bool Device_VK::HasShaderSubgroupBallot() const
{
	return m_shaderSubgroupBallot;
}

bool Device_VK::HasShaderSubgroupVote() const
{
	return m_shaderSubgroupVote;
}

bool Device_VK::HasShaderUniformBufferArrayDynamicIndexing() const
{
	return m_shaderUniformBufferArrayDynamicIndexing;
}

bool Device_VK::HasShaderSampledImageArrayDynamicIndexing() const
{
	return m_shaderSampledImageArrayDynamicIndexing;
}

bool Device_VK::HasShaderStorageBufferArrayDynamicIndexing() const
{
	return m_shaderStorageBufferArrayDynamicIndexing;
}

bool Device_VK::HasShaderStorageImageArrayDynamicIndexing() const
{
	return m_shaderStorageImageArrayDynamicIndexing;
}

bool Device_VK::HasShaderInputAttachmentArrayDynamicIndexing() const
{
	return m_shaderInputAttachmentArrayDynamicIndexing;
}

bool Device_VK::HasShaderUniformTexelBufferArrayDynamicIndexing() const
{
	return m_shaderUniformTexelBufferArrayDynamicIndexing;
}

bool Device_VK::HasShaderStorageTexelBufferArrayDynamicIndexing() const
{
	return m_shaderStorageTexelBufferArrayDynamicIndexing;
}

bool Device_VK::HasShaderUniformBufferArrayNonUniformIndexing() const
{
	return m_shaderUniformBufferArrayNonUniformIndexing;
}

bool Device_VK::HasShaderSampledImageArrayNonUniformIndexing() const
{
	return m_shaderSampledImageArrayNonUniformIndexing;
}

bool Device_VK::HasShaderStorageBufferArrayNonUniformIndexing() const
{
	return m_shaderStorageBufferArrayNonUniformIndexing;
}

bool Device_VK::HasShaderStorageImageArrayNonUniformIndexing() const
{
	return m_shaderStorageImageArrayNonUniformIndexing;
}

bool Device_VK::HasShaderInputAttachmentArrayNonUniformIndexing() const
{
	return m_shaderInputAttachmentArrayNonUniformIndexing;
}

bool Device_VK::HasShaderUniformTexelBufferArrayNonUniformIndexing() const
{
	return m_shaderUniformTexelBufferArrayNonUniformIndexing;
}

bool Device_VK::HasShaderStorageTexelBufferArrayNonUniformIndexing() const
{
	return m_shaderStorageTexelBufferArrayNonUniformIndexing;
}

bool Device_VK::HasShaderInt16() const
{
	return m_shaderInt16;
}

bool Device_VK::HasShaderFloat64() const
{
	return m_shaderFloat64;
}

bool Device_VK::HasShaderStorageImageReadWithoutFormat() const
{
	return m_shaderStorageImageReadWithoutFormat;
}

bool Device_VK::HasShaderStorageImageExtendedFormats() const
{
	return m_shaderStorageImageExtendedFormats;
}

bool Device_VK::HasShaderStorageImageMultisample() const
{
	return m_shaderStorageImageMultisample;
}

bool Device_VK::HasImageFormatList() const
{
	return m_imageFormatList;
}

bool Device_VK::HasShaderClipDistance() const
{
	return m_shaderClipDistance;
}

bool Device_VK::HasShaderCullDistance() const
{
	return m_shaderCullDistance;
}

bool Device_VK::HasVertexPipelineStoresAndAtomics() const
{
	return m_vertexPipelineStoresAndAtomics;
}

bool Device_VK::HasSampleRateShading() const
{
	return m_sampleRateShading;
}

bool Device_VK::HasImageCubeArray() const
{
	return m_imageCubeArray;
}

bool Device_VK::HasTextureCompressionBC() const
{
	return m_textureCompressionBC;
}

bool Device_VK::HasDrawIndirectFirstInstance() const
{
	return m_drawIndirectFirstInstance;
}

bool Device_VK::HasScalarBlockLayout() const
{
	return m_scalarBlockLayout;
}

bool Device_VK::HasRuntimeDescriptorArray() const
{
	return m_runtimeDescriptorArray;
}

bool Device_VK::HasDescriptorBindingPartiallyBound() const
{
	return m_descriptorBindingPartiallyBound;
}

bool Device_VK::HasShaderOutputViewportIndex() const
{
	return m_shaderOutputViewportIndex;
}

bool Device_VK::HasShaderOutputLayer() const
{
	return m_shaderOutputLayer;
}

bool Device_VK::HasSubgroupBroadcastDynamicId() const
{
	return m_subgroupBroadcastDynamicId;
}

bool Device_VK::HasShaderBufferFloat32Atomics() const
{
	return m_shaderBufferFloat32Atomics;
}

bool Device_VK::HasShaderBufferFloat32AtomicAdd() const
{
	return m_shaderBufferFloat32AtomicAdd;
}

bool Device_VK::HasShaderBufferFloat64Atomics() const
{
	return m_shaderBufferFloat64Atomics;
}

bool Device_VK::HasShaderBufferFloat64AtomicAdd() const
{
	return m_shaderBufferFloat64AtomicAdd;
}

bool Device_VK::HasShaderSharedFloat32Atomics() const
{
	return m_shaderSharedFloat32Atomics;
}

bool Device_VK::HasShaderSharedFloat32AtomicAdd() const
{
	return m_shaderSharedFloat32AtomicAdd;
}

bool Device_VK::HasShaderSharedFloat64Atomics() const
{
	return m_shaderSharedFloat64Atomics;
}

bool Device_VK::HasShaderSharedFloat64AtomicAdd() const
{
	return m_shaderSharedFloat64AtomicAdd;
}

bool Device_VK::HasShaderImageFloat32Atomics() const
{
	return m_shaderImageFloat32Atomics;
}

bool Device_VK::HasShaderImageFloat32AtomicAdd() const
{
	return m_shaderImageFloat32AtomicAdd;
}

bool Device_VK::HasSparseImageFloat32Atomics() const
{
	return m_sparseImageFloat32Atomics;
}

bool Device_VK::HasSparseImageFloat32AtomicAdd() const
{
	return m_sparseImageFloat32AtomicAdd;
}

bool Device_VK::HasShaderBufferFloat16Atomics() const
{
	return m_shaderBufferFloat16Atomics;
}

bool Device_VK::HasShaderBufferFloat16AtomicAdd() const
{
	return m_shaderBufferFloat16AtomicAdd;
}

bool Device_VK::HasShaderBufferFloat16AtomicMinMax() const
{
	return m_shaderBufferFloat16AtomicMinMax;
}

bool Device_VK::HasShaderBufferFloat32AtomicMinMax() const
{
	return m_shaderBufferFloat32AtomicMinMax;
}

bool Device_VK::HasShaderBufferFloat64AtomicMinMax() const
{
	return m_shaderBufferFloat64AtomicMinMax;
}

bool Device_VK::HasShaderSharedFloat16Atomics() const
{
	return m_shaderSharedFloat16Atomics;
}

bool Device_VK::HasShaderSharedFloat16AtomicAdd() const
{
	return m_shaderSharedFloat16AtomicAdd;
}

bool Device_VK::HasShaderSharedFloat16AtomicMinMax() const
{
	return m_shaderSharedFloat16AtomicMinMax;
}

bool Device_VK::HasShaderSharedFloat32AtomicMinMax() const
{
	return m_shaderSharedFloat32AtomicMinMax;
}

bool Device_VK::HasShaderSharedFloat64AtomicMinMax() const
{
	return m_shaderSharedFloat64AtomicMinMax;
}

bool Device_VK::HasShaderImageFloat32AtomicMinMax() const
{
	return m_shaderImageFloat32AtomicMinMax;
}

bool Device_VK::HasSparseImageFloat32AtomicMinMax() const
{
	return m_sparseImageFloat32AtomicMinMax;
}

bool Device_VK::HasShaderSubgroupClock() const
{
	return m_shaderSubgroupClock;
}

bool Device_VK::HasShaderDeviceClock() const
{
	return m_shaderDeviceClock;
}

bool Device_VK::HasHlslFunctionality1() const
{
	return m_hlslFunctionality1;
}

bool Device_VK::HasDeferredHostOperations() const
{
	return m_deferredHostOperations;
}

bool Device_VK::HasPipelineLibrary() const
{
	return m_pipelineLibrary;
}

bool Device_VK::HasSpirv14() const
{
	return m_spirv14;
}

bool Device_VK::HasShaderFloatControls() const
{
	return m_shaderFloatControls;
}

bool Device_VK::HasAccelerationStructure() const
{
	return m_accelerationStructure;
}

bool Device_VK::HasAccelerationStructureCaptureReplay() const
{
	return m_accelerationStructureCaptureReplay;
}

bool Device_VK::HasAccelerationStructureIndirectBuild() const
{
	return m_accelerationStructureIndirectBuild;
}

bool Device_VK::HasAccelerationStructureHostCommands() const
{
	return m_accelerationStructureHostCommands;
}

bool Device_VK::HasRayTracingPipeline() const
{
	return m_rayTracingPipeline;
}

bool Device_VK::HasRayTracingPipelineShaderGroupHandleCaptureReplay() const
{
	return m_rayTracingPipelineShaderGroupHandleCaptureReplay;
}

bool Device_VK::HasRayTracingPipelineShaderGroupHandleCaptureReplayMixed() const
{
	return m_rayTracingPipelineShaderGroupHandleCaptureReplayMixed;
}

bool Device_VK::HasRayTracingPipelineTraceRaysIndirect() const
{
	return m_rayTracingPipelineTraceRaysIndirect;
}

bool Device_VK::HasRayTraversalPrimitiveCulling() const
{
	return m_rayTraversalPrimitiveCulling;
}

bool Device_VK::HasRayQuery() const
{
	return m_rayQuery;
}

bool Device_VK::HasRayTracingMaintenance1() const
{
	return m_rayTracingMaintenance1;
}

bool Device_VK::HasRayTracingPipelineTraceRaysIndirect2() const
{
	return m_rayTracingPipelineTraceRaysIndirect2;
}

const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& Device_VK::GetRayTracingPipelineProperties() const
{
	return m_rtPipelineProps;
}

uint32_t Device_VK::GetShaderGroupHandleSize() const
{
	return m_rtPipelineProps.shaderGroupHandleSize;
}

uint32_t Device_VK::GetShaderGroupHandleAlignment() const
{
	return m_rtPipelineProps.shaderGroupHandleAlignment;
}

uint32_t Device_VK::GetShaderGroupBaseAlignment() const
{
	return m_rtPipelineProps.shaderGroupBaseAlignment;
}

uint32_t Device_VK::GetMaxRayRecursionDepth() const
{
	return m_rtPipelineProps.maxRayRecursionDepth;
}

bool Device_VK::HasRayTracingPositionFetch() const
{
	return m_rayTracingPositionFetch;
}

bool Device_VK::HasMicromap() const
{
	return m_micromap;
}

bool Device_VK::HasOpacityMicromapKhr() const
{
	return m_micromapKhr;
}

bool Device_VK::HasMicromapCaptureReplay() const
{
	return m_micromapCaptureReplay;
}

bool Device_VK::HasMicromapHostCommands() const
{
	return m_micromapHostCommands;
}

bool Device_VK::HasIndexTypeUint8() const
{
	return m_indexTypeUint8;
}

bool Device_VK::HasRayTracingInvocationReorder() const
{
	return m_rayTracingInvocationReorder;
}

bool Device_VK::HasPipelineLibraryGroupHandles() const
{
	return m_pipelineLibraryGroupHandles;
}

bool Device_VK::HasCooperativeMatrix() const
{
	return m_cooperativeMatrix;
}

bool Device_VK::HasCooperativeMatrixRobustBufferAccess() const
{
	return m_cooperativeMatrixRobustBufferAccess;
}

bool Device_VK::HasShaderBFloat16Type() const
{
	return m_shaderBFloat16Type;
}

bool Device_VK::HasShaderBFloat16DotProduct() const
{
	return m_shaderBFloat16DotProduct;
}

bool Device_VK::HasShaderBFloat16CooperativeMatrix() const
{
	return m_shaderBFloat16CooperativeMatrix;
}

bool Device_VK::HasShaderFloat8() const
{
	return m_shaderFloat8;
}

bool Device_VK::HasShaderFloat8CooperativeMatrix() const
{
	return m_shaderFloat8CooperativeMatrix;
}

bool Device_VK::HasWorkgroupMemoryExplicitLayout() const
{
	return m_workgroupMemoryExplicitLayout;
}

bool Device_VK::HasWorkgroupMemoryExplicitLayoutScalarBlockLayout() const
{
	return m_workgroupMemoryExplicitLayoutScalarBlockLayout;
}

bool Device_VK::HasWorkgroupMemoryExplicitLayout8BitAccess() const
{
	return m_workgroupMemoryExplicitLayout8BitAccess;
}

bool Device_VK::HasWorkgroupMemoryExplicitLayout16BitAccess() const
{
	return m_workgroupMemoryExplicitLayout16BitAccess;
}

bool Device_VK::HasLongVector() const
{
	return m_longVector;
}

uint32_t Device_VK::MaxLongVectorComponents() const
{
	return m_maxLongVectorComponents;
}

bool Device_VK::HasCooperativeMatrixProperties2() const
{
	return m_cooperativeMatrixProperties2;
}

bool Device_VK::HasCooperativeMatrixReductions() const
{
	return m_cooperativeMatrixReductions;
}

bool Device_VK::HasCooperativeMatrixConversions() const
{
	return m_cooperativeMatrixConversions;
}

bool Device_VK::HasCooperativeMatrixPerElementOperations() const
{
	return m_cooperativeMatrixPerElementOperations;
}

bool Device_VK::HasCooperativeMatrixGetCoordinate() const
{
	return m_cooperativeMatrixGetCoordinate;
}

uint32_t Device_VK::SubgroupSize() const
{
	return m_subgroupSize;
}

VkShaderStageFlags Device_VK::SubgroupStages() const
{
	return m_subgroupStages;
}

VkSubgroupFeatureFlags Device_VK::SubgroupOperations() const
{
	return m_subgroupOperations;
}

bool Device_VK::SubgroupQuadOperationsInAllStages() const
{
	return m_subgroupQuadOperationsInAllStages;
}

bool Device_VK::HasMeshShader() const
{
	return m_meshShader;
}

bool Device_VK::HasFragmentShadingRate() const
{
	return m_fragmentShadingRate;
}

bool Device_VK::HasConditionalRendering() const
{
	return m_conditionalRendering;
}

bool Device_VK::HasTransformFeedback() const
{
	return m_transformFeedback;
}

bool Device_VK::HasGeometryStreams() const
{
	return m_geometryStreams;
}

bool Device_VK::HasSampleLocations() const
{
	return m_sampleLocations;
}

bool Device_VK::VariableSampleLocations() const
{
	return m_variableSampleLocations;
}

bool Device_VK::SampleLocationGrid(uint32_t sampleCount, uint32_t& width, uint32_t& height) const
{
	auto index = 0u;
	auto count = sampleCount;
	if (count < 1 || count > 16 || (count & (count - 1)) != 0) return false;
	while (count > 1)
	{
		count >>= 1;
		++index;
	}

	width = m_sampleLocationGrids[index].width;
	height = m_sampleLocationGrids[index].height;

	return width != 0 && height != 0;
}

void Device_VK::SampleLocationRange(float& minimum, float& maximum, uint32_t& subPixelBits) const
{
	minimum = m_sampleLocationRange[0];
	maximum = m_sampleLocationRange[1];
	subPixelBits = m_sampleLocationSubPixelBits;
}

bool Device_VK::HasConservativeRasterization() const
{
	return m_conservativeRaster;
}

bool Device_VK::HasSparseBinding() const
{
	return m_sparseBinding;
}

bool Device_VK::SupportsSparseBuffer() const
{
	return m_sparseBinding && m_sparseResidencyBuffer;
}

bool Device_VK::SupportsSparseImage(bool is3D, uint32_t sampleCount) const
{
	if (!m_sparseBinding) return false;
	if (is3D) return m_sparseResidencyImage3D;

	if (!m_sparseResidencyImage2D) return false;
	if (sampleCount <= 1) return true;
	if (sampleCount == 2) return m_sparseResidency2Samples;
	if (sampleCount == 4) return m_sparseResidency4Samples;
	if (sampleCount == 8) return m_sparseResidency8Samples;
	if (sampleCount == 16) return m_sparseResidency16Samples;

	return false;
}

bool Device_VK::SupportsSparseAliased() const
{
	return m_sparseBinding && m_sparseResidencyAliased;
}

bool Device_VK::GetSparseQueue(VkQueue& queue) const
{
	queue = VK_NULL_HANDLE;
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &familyCount, nullptr);
	vector<VkQueueFamilyProperties> families(familyCount);
	if (familyCount) vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &familyCount, families.data());

	for (auto i = 0u; i < m_poolCount; ++i)
	{
		const auto family = m_queuePools[i].Family;
		if (family >= familyCount || (families[family].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) == 0) continue;

		vkGetDeviceQueue(m_device, family, 0, &queue);

		return queue != VK_NULL_HANDLE;
	}

	return false;
}

bool Device_VK::QueueSparseBind(VkQueue queue, VkBindSparseInfo bind) const
{
	assert(queue != VK_NULL_HANDLE);
	if (queue == VK_NULL_HANDLE) return false;

	{
		lock_guard<mutex> lock(m_queueMutex);
		if (m_sparseTimeline == VK_NULL_HANDLE)
		{
			VkSemaphoreTypeCreateInfo type = {};
			type.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
			type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
			VkSemaphoreCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			info.pNext = &type;
			if (vkCreateSemaphore(m_device, &info, nullptr, &m_sparseTimeline) < 0)
			{
				assert(m_sparseTimeline != VK_NULL_HANDLE);
				return false;
			}
		}
	}

	uint64_t waitValue = 0;
	uint64_t signalValue = 0;
	{
		lock_guard<mutex> lock(m_queueMutex);
		waitValue = m_sparseValue;
		signalValue = m_sparseValue + 1;
		++m_sparseValue;
	}

	VkTimelineSemaphoreSubmitInfo timeline = {};
	timeline.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	timeline.waitSemaphoreValueCount = waitValue ? 1u : 0u;
	timeline.pWaitSemaphoreValues = waitValue ? &waitValue : nullptr;
	timeline.signalSemaphoreValueCount = 1;
	timeline.pSignalSemaphoreValues = &signalValue;
	bind.pNext = &timeline;
	bind.waitSemaphoreCount = waitValue ? 1u : 0u;
	bind.pWaitSemaphores = waitValue ? &m_sparseTimeline : nullptr;
	bind.signalSemaphoreCount = 1;
	bind.pSignalSemaphores = &m_sparseTimeline;

	const auto result = vkQueueBindSparse(queue, 1, &bind, VK_NULL_HANDLE);
	if (result < 0)
	{
		cerr << "VkResult " << result << endl;
		assert(result >= 0);
		VkTimelineSemaphoreSubmitInfo signalTimeline = {};
		signalTimeline.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		signalTimeline.signalSemaphoreValueCount = 1;
		signalTimeline.pSignalSemaphoreValues = &signalValue;
		VkBindSparseInfo signalOnly = {};
		signalOnly.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
		signalOnly.pNext = &signalTimeline;
		signalOnly.signalSemaphoreCount = 1;
		signalOnly.pSignalSemaphores = &m_sparseTimeline;
		vkQueueBindSparse(queue, 1, &signalOnly, VK_NULL_HANDLE);

		return false;
	}

	return true;
}

bool Device_VK::SubmitQueue(VkQueue queue, const VkSubmitInfo2& info, VkFence fence,
	VkSemaphore* pWaitSemaphores, VkPipelineStageFlags* pWaitStages, uint64_t* pWaitValues,
	VkCommandBuffer* pCommandBuffers, VkSemaphore* pSignalSemaphores, uint64_t* pSignalValues) const
{
	return SubmitQueueInfo(m_synchronization2, queue, info, fence, pWaitSemaphores, pWaitStages, pWaitValues,
		pCommandBuffers, pSignalSemaphores, pSignalValues);
}

void Device_VK::ReadSparseTimeline(VkSemaphore& semaphore, uint64_t& value) const
{
	lock_guard<mutex> lock(m_queueMutex);
	semaphore = m_sparseTimeline;
	value = m_sparseValue;
}

bool Device_VK::EmulatesD24S8() const
{
	return m_emulateD24S8;
}

uint32_t Device_VK::FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memory;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memory);

	auto match = [&](VkMemoryPropertyFlags required)
	{
		for (auto i = 0u; i < memory.memoryTypeCount; ++i)
			if ((typeBits & (1u << i)) && (memory.memoryTypes[i].propertyFlags & required) == required)
				return i;

		return UINT32_MAX;
	};

	auto index = match(properties);
	if (index == UINT32_MAX && (properties & VK_MEMORY_PROPERTY_HOST_CACHED_BIT))
		index = match(properties & ~VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

	return index;
}

void Device_VK::SetDebugName(VkObjectType type, uint64_t handle, const wchar_t* name) const
{
	if (!m_debugUtils || !name || m_device == VK_NULL_HANDLE) return;

	const auto utf8 = WideToUtf8(name);
	VkDebugUtilsObjectNameInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	info.objectType = type;
	info.objectHandle = handle;
	info.pObjectName = utf8.c_str();
	if (m_ext.SetDebugName) m_ext.SetDebugName(m_device, &info);
}

bool Device_VK::createInstance()
{
	uint32_t loaderVersion = VK_API_VERSION_1_0;
	const auto enumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
		vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
	if (enumerateInstanceVersion) enumerateInstanceVersion(&loaderVersion);
	if (loaderVersion >= VK_API_VERSION_1_3) m_apiVersion = VK_API_VERSION_1_3;
	else if (loaderVersion >= VK_API_VERSION_1_2) m_apiVersion = VK_API_VERSION_1_2;
	else if (loaderVersion >= VK_API_VERSION_1_1) m_apiVersion = VK_API_VERSION_1_1;
	else m_apiVersion = VK_API_VERSION_1_0;

	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	vector<VkExtensionProperties> exts(extCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.data());

	vector<const char*> enabled;
	enabled.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	enabled.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	if (m_apiVersion < VK_API_VERSION_1_1)
	{
		if (!HasExtension(exts, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			cerr << "Vulkan extension unavailable: " << VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME << endl;

			return false;
		}
		enabled.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	}
	m_debugUtils = HasExtension(exts, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	if (m_debugUtils) enabled.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	// Enable the validation layer. It is optional, the same way the D3D12 debug layer is.
	vector<const char*> enabledLayers;
#if defined(_DEBUG)
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	vector<VkLayerProperties> layers(layerCount);
	if (layerCount) vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
	if (HasLayer(layers, "VK_LAYER_KHRONOS_validation"))
		enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

	VkApplicationInfo app = {};
	app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pApplicationName = "XUSG";
	app.apiVersion = m_apiVersion;

	VkInstanceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info.pApplicationInfo = &app;
	info.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
	info.ppEnabledLayerNames = enabledLayers.data();
	info.enabledExtensionCount = static_cast<uint32_t>(enabled.size());
	info.ppEnabledExtensionNames = enabled.data();

	XUSG_VK_CHECK(vkCreateInstance(&info, nullptr, &m_instance), cerr, false);
	m_ownsInstance = true;

	return true;
}

bool Device_VK::pickPhysicalDevice(VkPhysicalDevice requested)
{
	if (requested != VK_NULL_HANDLE)
	{
		m_physicalDevice = requested;

		return true;
	}

	uint32_t count = 0;
	vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
	if (!count) return false;
	vector<VkPhysicalDevice> devices(count);
	vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

	for (const auto device : devices)
	{
		VkPhysicalDeviceProperties props = {};
		vkGetPhysicalDeviceProperties(device, &props);

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			m_physicalDevice = device;

			return true;
		}
	}

	m_physicalDevice = devices[0];

	return m_physicalDevice != VK_NULL_HANDLE;
}

bool Device_VK::createDevice()
{
	// vkd3d-proton does the same: AMD (and some other drivers) do not expose
	// D24S8 as a depth attachment, so the D24 family is stored as D32S8.
	VkFormatProperties d24s8 = {};
	vkGetPhysicalDeviceFormatProperties(m_physicalDevice, VK_FORMAT_D24_UNORM_S8_UINT, &d24s8);
	m_emulateD24S8 = (d24s8.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0;

	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &familyCount, nullptr);
	vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &familyCount, families.data());

	const auto singleQueue = SingleQueueRequested();
	auto graphicsFamily = FindQueueFamily(families, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT,
		VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
	auto computeFamily = singleQueue ? graphicsFamily : FindQueueFamily(families,
		VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, VK_QUEUE_COMPUTE_BIT);
	auto copyFamily = singleQueue ? graphicsFamily : FindQueueFamily(families,
		VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, VK_QUEUE_TRANSFER_BIT);
	if (computeFamily == UINT32_MAX) computeFamily = graphicsFamily;
	if (copyFamily == UINT32_MAX) copyFamily = computeFamily;
	if (graphicsFamily == UINT32_MAX) return false;
	m_graphicsFamily = graphicsFamily;

	auto addPool = [&](uint32_t family)
	{
		for (auto i = 0u; i < m_poolCount; ++i)
			if (m_queuePools[i].Family == family) return i;

		auto& pool = m_queuePools[m_poolCount];
		pool.Family = family;
		pool.Count = singleQueue ? 1u : (min)(families[family].queueCount, 4u);
		assert(pool.Count);

		return m_poolCount++;
	};
	m_poolCount = 0;
	m_typePool[0] = addPool(graphicsFamily);
	m_typePool[1] = addPool(computeFamily);
	m_typePool[2] = addPool(copyFamily);
	m_sharingCount = 0;
	for (auto i = 0u; i < m_poolCount; ++i) m_sharingFamilies[m_sharingCount++] = m_queuePools[i].Family;

	uint32_t extCount = 0;
	vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extCount, nullptr);
	vector<VkExtensionProperties> exts(extCount);
	vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extCount, exts.data());

	auto require = [&](const char* name)
	{
		const auto supported = HasExtension(exts, name);
		if (!supported) cerr << "Vulkan extension unavailable: " << name << endl;

		return supported;
	};

	VkPhysicalDeviceProperties props = {};
	vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
	const auto instanceVersion = m_apiVersion;
	if (props.apiVersion < m_apiVersion) m_apiVersion = props.apiVersion;
	const auto core11 = m_apiVersion >= VK_API_VERSION_1_1;
	const auto core12 = m_apiVersion >= VK_API_VERSION_1_2;
	const auto coreDynamicRendering = m_apiVersion >= VK_API_VERSION_1_3;

	vector<const char*> enabled;
	// Core extensions with no fallback. Missing one fails device creation.
	// A name promoted into the requested API version is already core.
	XUSG_N_RETURN(require(VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME), false);
	XUSG_N_RETURN(require(VK_KHR_SWAPCHAIN_EXTENSION_NAME), false);
	const char* robustName = nullptr;
	if (HasExtension(exts, VK_KHR_ROBUSTNESS_2_EXTENSION_NAME))
		robustName = VK_KHR_ROBUSTNESS_2_EXTENSION_NAME;
	else if (HasExtension(exts, VK_EXT_ROBUSTNESS_2_EXTENSION_NAME))
		robustName = VK_EXT_ROBUSTNESS_2_EXTENSION_NAME;
	m_nullDescriptor = robustName != nullptr;
	m_deviceGeneratedCommands = HasExtension(exts, VK_EXT_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME);
	m_maintenance5 = HasExtension(exts, VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
	const char* untypedName = nullptr;
	if (HasExtension(exts, VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME))
		untypedName = VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME;
	m_shaderUntypedPointers = untypedName != nullptr;
	const char* divisorName = nullptr;
	if (HasExtension(exts, VK_KHR_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME))
		divisorName = VK_KHR_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME;
	else if (HasExtension(exts, VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME))
		divisorName = VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME;
	m_vertexAttributeDivisor = divisorName != nullptr;
	const auto dynamicRenderingExt = HasExtension(exts, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
	const auto sync2Ext = HasExtension(exts, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	const auto extendedDynamicStateExt = HasExtension(exts, VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
	if (!dynamicRenderingExt && !coreDynamicRendering)
		XUSG_N_RETURN(require(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME), false);
	if (!extendedDynamicStateExt && !coreDynamicRendering)
		XUSG_N_RETURN(require(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME), false);
	const auto depthResolveExt = HasExtension(exts, VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
	const auto bufferAddressExt = HasExtension(exts, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	const auto timelineExt = HasExtension(exts, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
	const auto float16Ext = HasExtension(exts, VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);
	if (!core12 && instanceVersion < VK_API_VERSION_1_2)
	{
		XUSG_N_RETURN(require(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME), false);
		XUSG_N_RETURN(require(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME), false);
		XUSG_N_RETURN(require(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME), false);
	}
	enabled.push_back(VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME);
	enabled.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	if (!core12 && bufferAddressExt) enabled.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	if (!core12 && timelineExt) enabled.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
	if (!core12 && float16Ext) enabled.push_back(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);
	if (dynamicRenderingExt) enabled.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
	// Core extensions that stay off when missing. Device creation still succeeds.
	m_conditionalRendering = HasExtension(exts, VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);
	m_transformFeedback = HasExtension(exts, VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME);
	m_conservativeRaster = HasExtension(exts, VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);
	m_customBorderColor = HasExtension(exts, VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);

	// Ultimate extensions. A missing extension leaves the feature off.
	m_meshShader = HasExtension(exts, VK_EXT_MESH_SHADER_EXTENSION_NAME);
	m_fragmentShadingRate = HasExtension(exts, VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
	m_sampleLocations = HasExtension(exts, VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME) &&
		(coreDynamicRendering || HasExtension(exts, VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME)) &&
		HasExtension(exts, VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
	auto enableExtendedDynamicState = false;
	auto meshTask = false;
	auto fsrPrimitive = false;
	auto fsrAttachment = false;

	m_heapProps = {};
	m_heapProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;
	m_dgcProps = {};
	m_dgcProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_EXT;
	m_dgcProps.pNext = &m_heapProps;

	VkPhysicalDeviceDescriptorHeapFeaturesEXT heapFeatures = {};
	heapFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT;
	VkPhysicalDeviceDeviceGeneratedCommandsFeaturesEXT dgcFeatures = {};
	dgcFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_EXT;
	VkPhysicalDeviceRobustness2FeaturesKHR robust = {};
	robust.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR;
	VkPhysicalDeviceVertexAttributeDivisorFeatures divisor = {};
	divisor.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES;
	void* featureNext = &heapFeatures;
	if (m_deviceGeneratedCommands)
	{
		dgcFeatures.pNext = featureNext;
		featureNext = &dgcFeatures;
	}
	if (m_nullDescriptor)
	{
		robust.pNext = featureNext;
		featureNext = &robust;
	}
	if (m_vertexAttributeDivisor)
	{
		divisor.pNext = featureNext;
		featureNext = &divisor;
	}
	VkPhysicalDeviceConditionalRenderingFeaturesEXT conditional = {};
	conditional.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
	if (m_conditionalRendering)
	{
		conditional.pNext = featureNext;
		featureNext = &conditional;
	}

	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

	VkPhysicalDeviceMaintenance5Features maintenance5 = {};
	maintenance5.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES;
	if (extendedDynamicStateExt)
	{
		dynamicState.pNext = featureNext;
		featureNext = &dynamicState;
	}
	if (m_maintenance5)
	{
		maintenance5.pNext = featureNext;
		featureNext = &maintenance5;
	}

	VkPhysicalDeviceShaderUntypedPointersFeaturesKHR untyped = {};
	untyped.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR;
	VkPhysicalDeviceTransformFeedbackFeaturesEXT xfbQuery = {};
	xfbQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT;
	void* queryTail = featureNext;
	if (m_shaderUntypedPointers)
	{
		untyped.pNext = queryTail;
		queryTail = &untyped;
	}
	if (m_transformFeedback)
	{
		xfbQuery.pNext = queryTail;
		queryTail = &xfbQuery;
	}
	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamicState3 = {};
	dynamicState3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
	VkPhysicalDeviceMeshShaderFeaturesEXT meshQuery = {};
	meshQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
	VkPhysicalDeviceFragmentShadingRateFeaturesKHR fsrQuery = {};
	fsrQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
	VkPhysicalDeviceCustomBorderColorFeaturesEXT borderQuery = {};
	borderQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingQuery = {};
	dynamicRenderingQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	VkPhysicalDeviceSynchronization2Features sync2Query = {};
	sync2Query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;

	void* queryHead = queryTail;
	if (m_sampleLocations)
	{
		dynamicState3.pNext = queryHead;
		queryHead = &dynamicState3;
	}
	if (m_meshShader)
	{
		meshQuery.pNext = queryHead;
		queryHead = &meshQuery;
	}
	if (m_fragmentShadingRate)
	{
		fsrQuery.pNext = queryHead;
		queryHead = &fsrQuery;
	}
	if (m_customBorderColor)
	{
		borderQuery.pNext = queryHead;
		queryHead = &borderQuery;
	}
	if (!coreDynamicRendering)
	{
		dynamicRenderingQuery.pNext = queryHead;
		queryHead = &dynamicRenderingQuery;
		if (sync2Ext)
		{
			sync2Query.pNext = queryHead;
			queryHead = &sync2Query;
		}
	}

	VkPhysicalDeviceFeatures2 features = {};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features.pNext = queryHead;
	vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features);
	if (!heapFeatures.descriptorHeap) return false;
	if (m_deviceGeneratedCommands && !dgcFeatures.deviceGeneratedCommands) m_deviceGeneratedCommands = false;
	if (m_deviceGeneratedCommands) enabled.push_back(VK_EXT_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME);
	if (m_nullDescriptor && !robust.nullDescriptor) m_nullDescriptor = false;
	if (m_nullDescriptor) enabled.push_back(robustName);
	if (m_maintenance5 && !maintenance5.maintenance5) m_maintenance5 = false;
	if (m_maintenance5) enabled.push_back(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
	if (m_conditionalRendering && !conditional.conditionalRendering) m_conditionalRendering = false;
	if (m_conditionalRendering) enabled.push_back(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);
	if (m_transformFeedback && !xfbQuery.transformFeedback) m_transformFeedback = false;
	if (m_transformFeedback) enabled.push_back(VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME);
	if (m_conservativeRaster) enabled.push_back(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);
	if (m_shaderUntypedPointers && !untyped.shaderUntypedPointers) m_shaderUntypedPointers = false;
	if (m_vertexAttributeDivisor && !divisor.vertexAttributeInstanceRateDivisor) m_vertexAttributeDivisor = false;
	if (m_shaderUntypedPointers) enabled.push_back(untypedName);
	if (m_vertexAttributeDivisor) enabled.push_back(divisorName);
	if (!coreDynamicRendering && !dynamicRenderingQuery.dynamicRendering) return false;
	enableExtendedDynamicState = extendedDynamicStateExt && dynamicState.extendedDynamicState == VK_TRUE;
	if (!enableExtendedDynamicState && !coreDynamicRendering)
	{
		cerr << "Vulkan extension unavailable: " << VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME << endl;

		return false;
	}
	if (enableExtendedDynamicState) enabled.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
	m_geometryStreams = m_transformFeedback && xfbQuery.geometryStreams == VK_TRUE;
	if (m_meshShader && !meshQuery.meshShader) m_meshShader = false;
	meshTask = m_meshShader && meshQuery.taskShader == VK_TRUE;
	if (m_fragmentShadingRate && !fsrQuery.pipelineFragmentShadingRate) m_fragmentShadingRate = false;
	fsrPrimitive = m_fragmentShadingRate && fsrQuery.primitiveFragmentShadingRate == VK_TRUE;
	fsrAttachment = m_fragmentShadingRate && fsrQuery.attachmentFragmentShadingRate == VK_TRUE;
	if (m_customBorderColor && (!borderQuery.customBorderColors || !borderQuery.customBorderColorWithoutFormat))
		m_customBorderColor = false;
	if (m_sampleLocations && !dynamicState3.extendedDynamicState3SampleLocationsEnable) m_sampleLocations = false;
	if (m_customBorderColor) enabled.push_back(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);
	if (m_meshShader) enabled.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	if (m_fragmentShadingRate) enabled.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
	if (m_sampleLocations)
	{
		enabled.push_back(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME);
		if (!coreDynamicRendering) enabled.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
		enabled.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
	}

	VkPhysicalDeviceSampleLocationsPropertiesEXT sampleLocProps = {};
	sampleLocProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT;
	VkPhysicalDeviceDepthStencilResolveProperties resolveProps = {};
	resolveProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES;
	VkPhysicalDeviceSubgroupProperties subgroupProps = {};
	VkPhysicalDeviceProperties2 props2 = {};
	props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	props2.pNext = m_deviceGeneratedCommands ? static_cast<void*>(&m_dgcProps) : static_cast<void*>(&m_heapProps);
	if (core12 || depthResolveExt)
	{
		resolveProps.pNext = props2.pNext;
		props2.pNext = &resolveProps;
	}
	if (m_sampleLocations)
	{
		sampleLocProps.pNext = props2.pNext;
		props2.pNext = &sampleLocProps;
	}
	if (instanceVersion >= VK_API_VERSION_1_1)
	{
		subgroupProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
		subgroupProps.pNext = props2.pNext;
		props2.pNext = &subgroupProps;
	}
	VkPhysicalDeviceShaderLongVectorPropertiesEXT longVectorProps = {};
	const auto longVectorExt = HasExtension(exts, VK_EXT_SHADER_LONG_VECTOR_EXTENSION_NAME);
	if (longVectorExt)
	{
		longVectorProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_LONG_VECTOR_PROPERTIES_EXT;
		longVectorProps.pNext = props2.pNext;
		props2.pNext = &longVectorProps;
	}
	vkGetPhysicalDeviceProperties2(m_physicalDevice, &props2);
	if (m_sampleLocations)
	{
		m_variableSampleLocations = sampleLocProps.variableSampleLocations == VK_TRUE;
		m_sampleLocationRange[0] = sampleLocProps.sampleLocationCoordinateRange[0];
		m_sampleLocationRange[1] = sampleLocProps.sampleLocationCoordinateRange[1];
		m_sampleLocationSubPixelBits = sampleLocProps.sampleLocationSubPixelBits;
		m_sampleLocationCounts = sampleLocProps.sampleLocationSampleCounts;
	}
	if (core12 || depthResolveExt)
	{
		m_depthResolveModes = resolveProps.supportedDepthResolveModes;
		m_stencilResolveModes = resolveProps.supportedStencilResolveModes;
		m_independentResolveNone = resolveProps.independentResolveNone == VK_TRUE;
		m_independentResolve = resolveProps.independentResolve == VK_TRUE;
		m_depthStencilResolve = m_depthResolveModes != 0 || m_stencilResolveModes != 0;
	}
	if (!core12 && depthResolveExt && m_depthStencilResolve)
		enabled.push_back(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
	if (instanceVersion >= VK_API_VERSION_1_1)
	{
		m_subgroupSize = subgroupProps.subgroupSize;
		m_subgroupStages = subgroupProps.supportedStages;
		m_subgroupOperations = subgroupProps.supportedOperations;
		m_subgroupQuadOperationsInAllStages = subgroupProps.quadOperationsInAllStages == VK_TRUE;
	}

	const auto imageStride = static_cast<uint32_t>(m_heapProps.imageDescriptorSize);
	const auto bufferStride = static_cast<uint32_t>(m_heapProps.bufferDescriptorSize);
	m_resourceStride = (max)(imageStride, bufferStride);
	m_samplerStride = static_cast<uint32_t>(m_heapProps.samplerDescriptorSize);
	if (!m_resourceStride || !m_samplerStride) return false;

	VkPhysicalDeviceShaderUntypedPointersFeaturesKHR untypedEnable = {};
	VkPhysicalDeviceDescriptorHeapFeaturesEXT heapEnable = {};
	heapEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT;
	heapEnable.descriptorHeap = VK_TRUE;
	if (m_shaderUntypedPointers)
	{
		untypedEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR;
		untypedEnable.shaderUntypedPointers = VK_TRUE;
		heapEnable.pNext = &untypedEnable;
	}

	VkPhysicalDeviceDeviceGeneratedCommandsFeaturesEXT dgcEnable = {};
	void* enableBase = &heapEnable;
	if (m_deviceGeneratedCommands)
	{
		dgcEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_EXT;
		dgcEnable.pNext = &heapEnable;
		dgcEnable.deviceGeneratedCommands = VK_TRUE;
		enableBase = &dgcEnable;
	}

	VkPhysicalDeviceVulkan13Features supported13 = {};
	VkPhysicalDeviceVulkan12Features supported12 = {};
	VkPhysicalDeviceVulkan11Features supported11 = {};
	VkPhysicalDeviceBufferDeviceAddressFeatures supportedAddress = {};
	VkPhysicalDeviceTimelineSemaphoreFeatures supportedTimeline = {};
	VkPhysicalDeviceShaderFloat16Int8Features supportedFloat16 = {};
	supported12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	VkPhysicalDeviceFeatures2 supported2 = {};
	supported2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	if (instanceVersion >= VK_API_VERSION_1_1)
	{
		supported11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		supported12.pNext = &supported11;
	}
	if (instanceVersion >= VK_API_VERSION_1_2)
	{
		if (instanceVersion >= VK_API_VERSION_1_3)
		{
			supported13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
			supported13.pNext = &supported12;
			supported2.pNext = &supported13;
		}
		else supported2.pNext = &supported12;
	}
	else
	{
		supportedAddress.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		supportedTimeline.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
		supportedTimeline.pNext = &supportedAddress;
		supportedFloat16.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;
		supportedFloat16.pNext = &supportedTimeline;
		supported2.pNext = &supportedFloat16;
		if (instanceVersion >= VK_API_VERSION_1_1) supportedAddress.pNext = &supported11;
	}
	vkGetPhysicalDeviceFeatures2(m_physicalDevice, &supported2);
	const auto shaderFloat16 = instanceVersion >= VK_API_VERSION_1_2 ? supported12.shaderFloat16 : supportedFloat16.shaderFloat16;
	const auto bufferDeviceAddress = instanceVersion >= VK_API_VERSION_1_2 ?
		supported12.bufferDeviceAddress : supportedAddress.bufferDeviceAddress;
	const auto timelineSemaphore = instanceVersion >= VK_API_VERSION_1_2 ?
		supported12.timelineSemaphore : supportedTimeline.timelineSemaphore;
	if (!shaderFloat16 || !bufferDeviceAddress || !timelineSemaphore) return false;
	if (coreDynamicRendering && !supported13.dynamicRendering) return false;
	if (coreDynamicRendering) m_synchronization2 = supported13.synchronization2 == VK_TRUE;
	else m_synchronization2 = sync2Ext && sync2Query.synchronization2 == VK_TRUE;
	if (m_synchronization2 && sync2Ext) enabled.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

	// Shader-model and subgroup features. A missing bit or extension leaves that feature off.
	const auto storage16Ext = HasExtension(exts, VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
	const auto drawParamExt = HasExtension(exts, VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);
	const auto storage8Ext = HasExtension(exts, VK_KHR_8BIT_STORAGE_EXTENSION_NAME);
	const auto atomicInt64Ext = HasExtension(exts, VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME);
	const auto subgroupExtTypesExt = HasExtension(exts, VK_KHR_SHADER_SUBGROUP_EXTENDED_TYPES_EXTENSION_NAME);
	const auto subgroupSizeExt = HasExtension(exts, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);
	const auto integerDotExt = HasExtension(exts, VK_KHR_SHADER_INTEGER_DOT_PRODUCT_EXTENSION_NAME);
	const auto demoteExt = HasExtension(exts, VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME);
	const auto subgroupRotateExt = HasExtension(exts, VK_KHR_SHADER_SUBGROUP_ROTATE_EXTENSION_NAME);
	const auto floatControls2Ext = HasExtension(exts, VK_KHR_SHADER_FLOAT_CONTROLS_2_EXTENSION_NAME);
	const auto subgroupPartitionedExt = HasExtension(exts, VK_EXT_SHADER_SUBGROUP_PARTITIONED_EXTENSION_NAME);
	const auto quadControlExt = HasExtension(exts, VK_KHR_SHADER_QUAD_CONTROL_EXTENSION_NAME);
	const auto uniformControlFlowExt = HasExtension(exts, VK_KHR_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_EXTENSION_NAME);
	const auto maximalReconvergenceExt = HasExtension(exts, VK_KHR_SHADER_MAXIMAL_RECONVERGENCE_EXTENSION_NAME);
	const auto computeDerivativesExt = HasExtension(exts, VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME);
	const auto barycentricExt = HasExtension(exts, VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
	const auto imageAtomicInt64Ext = HasExtension(exts, VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME);
	const auto shaderAtomicFloatExt = HasExtension(exts, VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME);
	const auto shaderAtomicFloat2Ext = HasExtension(exts, VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME) &&
		shaderAtomicFloatExt;
	const auto shaderClockExt = HasExtension(exts, VK_KHR_SHADER_CLOCK_EXTENSION_NAME);
	m_hlslFunctionality1 = HasExtension(exts, VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME);
	const auto descriptorIndexingExt = HasExtension(exts, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	const auto scalarBlockLayoutExt = HasExtension(exts, VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
	const auto viewportIndexLayerExt = HasExtension(exts, VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME);
	const auto deferredHostOpsExt = core11 && HasExtension(exts, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	const auto accelerationStructureExt = deferredHostOpsExt &&
		HasExtension(exts, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	const auto pipelineLibraryExt = core11 && HasExtension(exts, VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
	const auto shaderFloatControlsExt = !core12 && HasExtension(exts, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	const auto spirv14Ext = shaderFloatControlsExt && HasExtension(exts, VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	m_deferredHostOperations = deferredHostOpsExt;
	m_pipelineLibrary = pipelineLibraryExt;
	m_shaderFloatControls = core12 || shaderFloatControlsExt;
	m_spirv14 = core12 || spirv14Ext;
	const auto rayTracingPipelineExt = core11 && HasExtension(exts, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
	const auto rayQueryExt = core11 && HasExtension(exts, VK_KHR_RAY_QUERY_EXTENSION_NAME);
	const auto rayTracingMaintenance1Ext = core11 &&
		HasExtension(exts, VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME);
	const auto rayTracingPositionFetchExt = core11 &&
		HasExtension(exts, VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME);
	const char* opacityMicromapExtName = nullptr;
	auto opacityMicromapKhr = false;
	if (core11 && HasExtension(exts, VK_KHR_OPACITY_MICROMAP_EXTENSION_NAME))
	{
		opacityMicromapExtName = VK_KHR_OPACITY_MICROMAP_EXTENSION_NAME;
		opacityMicromapKhr = true;
	}
	else if (core11 && HasExtension(exts, VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME))
		opacityMicromapExtName = VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME;
	const auto indexTypeUint8Ext = HasExtension(exts, VK_KHR_INDEX_TYPE_UINT8_EXTENSION_NAME);
	const auto invocationReorderExt = core11 &&
		HasExtension(exts, VK_EXT_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME);
	const char* pipelineLibraryGroupHandlesExtName = nullptr;
	if (core11 && HasExtension(exts, VK_KHR_PIPELINE_LIBRARY_GROUP_HANDLES_EXTENSION_NAME))
		pipelineLibraryGroupHandlesExtName = VK_KHR_PIPELINE_LIBRARY_GROUP_HANDLES_EXTENSION_NAME;
	else if (core11 && HasExtension(exts, VK_EXT_PIPELINE_LIBRARY_GROUP_HANDLES_EXTENSION_NAME))
		pipelineLibraryGroupHandlesExtName = VK_EXT_PIPELINE_LIBRARY_GROUP_HANDLES_EXTENSION_NAME;
	const auto cooperativeMatrixExt = HasExtension(exts, VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME);
	const auto shaderBfloat16Ext = HasExtension(exts, VK_KHR_SHADER_BFLOAT16_EXTENSION_NAME);
	const auto shaderFloat8Ext = HasExtension(exts, VK_EXT_SHADER_FLOAT8_EXTENSION_NAME);
	const auto workgroupLayoutExt = HasExtension(exts, VK_KHR_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_EXTENSION_NAME);
	const auto cooperativeMatrixMaintenance1Ext = cooperativeMatrixExt &&
		HasExtension(exts, VK_EXT_COOPERATIVE_MATRIX_MAINTENANCE_1_EXTENSION_NAME);
	m_shaderInt64 = features.features.shaderInt64 == VK_TRUE;
	m_shaderInt16 = features.features.shaderInt16 == VK_TRUE;
	m_shaderFloat64 = features.features.shaderFloat64 == VK_TRUE;
	m_shaderStorageImageReadWithoutFormat = features.features.shaderStorageImageReadWithoutFormat == VK_TRUE;
	m_shaderStorageImageExtendedFormats = features.features.shaderStorageImageExtendedFormats == VK_TRUE;
	m_shaderStorageImageMultisample = features.features.shaderStorageImageMultisample == VK_TRUE;
	m_shaderClipDistance = features.features.shaderClipDistance == VK_TRUE;
	m_shaderCullDistance = features.features.shaderCullDistance == VK_TRUE;
	m_vertexPipelineStoresAndAtomics = features.features.vertexPipelineStoresAndAtomics == VK_TRUE;
	m_sampleRateShading = features.features.sampleRateShading == VK_TRUE;
	m_imageCubeArray = features.features.imageCubeArray == VK_TRUE;
	m_textureCompressionBC = features.features.textureCompressionBC == VK_TRUE;
	m_drawIndirectFirstInstance = features.features.drawIndirectFirstInstance == VK_TRUE;
	m_shaderUniformBufferArrayDynamicIndexing = features.features.shaderUniformBufferArrayDynamicIndexing == VK_TRUE;
	m_shaderSampledImageArrayDynamicIndexing = features.features.shaderSampledImageArrayDynamicIndexing == VK_TRUE;
	m_shaderStorageBufferArrayDynamicIndexing = features.features.shaderStorageBufferArrayDynamicIndexing == VK_TRUE;
	m_shaderStorageImageArrayDynamicIndexing = features.features.shaderStorageImageArrayDynamicIndexing == VK_TRUE;
	if (instanceVersion >= VK_API_VERSION_1_1)
	{
		m_storageBuffer16BitAccess = supported11.storageBuffer16BitAccess == VK_TRUE;
		m_uniformAndStorageBuffer16BitAccess = supported11.uniformAndStorageBuffer16BitAccess == VK_TRUE;
		m_storagePushConstant16 = supported11.storagePushConstant16 == VK_TRUE;
		m_storageInputOutput16 = supported11.storageInputOutput16 == VK_TRUE;
		m_shaderDrawParameters = supported11.shaderDrawParameters == VK_TRUE;
	}
	if (instanceVersion >= VK_API_VERSION_1_2)
	{
		m_shaderInt8 = supported12.shaderInt8 == VK_TRUE;
		m_storageBuffer8BitAccess = supported12.storageBuffer8BitAccess == VK_TRUE;
		m_uniformAndStorageBuffer8BitAccess = supported12.uniformAndStorageBuffer8BitAccess == VK_TRUE;
		m_storagePushConstant8 = supported12.storagePushConstant8 == VK_TRUE;
		m_shaderBufferInt64Atomics = supported12.shaderBufferInt64Atomics == VK_TRUE;
		m_shaderSharedInt64Atomics = supported12.shaderSharedInt64Atomics == VK_TRUE;
		m_shaderSubgroupExtendedTypes = supported12.shaderSubgroupExtendedTypes == VK_TRUE;
		m_shaderInputAttachmentArrayDynamicIndexing = supported12.shaderInputAttachmentArrayDynamicIndexing == VK_TRUE;
		m_shaderUniformTexelBufferArrayDynamicIndexing = supported12.shaderUniformTexelBufferArrayDynamicIndexing == VK_TRUE;
		m_shaderStorageTexelBufferArrayDynamicIndexing = supported12.shaderStorageTexelBufferArrayDynamicIndexing == VK_TRUE;
		m_shaderUniformBufferArrayNonUniformIndexing = supported12.shaderUniformBufferArrayNonUniformIndexing == VK_TRUE;
		m_shaderSampledImageArrayNonUniformIndexing = supported12.shaderSampledImageArrayNonUniformIndexing == VK_TRUE;
		m_shaderStorageBufferArrayNonUniformIndexing = supported12.shaderStorageBufferArrayNonUniformIndexing == VK_TRUE;
		m_shaderStorageImageArrayNonUniformIndexing = supported12.shaderStorageImageArrayNonUniformIndexing == VK_TRUE;
		m_shaderInputAttachmentArrayNonUniformIndexing = supported12.shaderInputAttachmentArrayNonUniformIndexing == VK_TRUE;
		m_shaderUniformTexelBufferArrayNonUniformIndexing = supported12.shaderUniformTexelBufferArrayNonUniformIndexing == VK_TRUE;
		m_shaderStorageTexelBufferArrayNonUniformIndexing = supported12.shaderStorageTexelBufferArrayNonUniformIndexing == VK_TRUE;
		m_runtimeDescriptorArray = supported12.runtimeDescriptorArray == VK_TRUE;
		m_descriptorBindingPartiallyBound = supported12.descriptorBindingPartiallyBound == VK_TRUE;
		m_scalarBlockLayout = supported12.scalarBlockLayout == VK_TRUE;
		m_shaderOutputViewportIndex = supported12.shaderOutputViewportIndex == VK_TRUE;
		m_shaderOutputLayer = supported12.shaderOutputLayer == VK_TRUE;
		m_subgroupBroadcastDynamicId = supported12.subgroupBroadcastDynamicId == VK_TRUE;
	}
	else m_shaderInt8 = supportedFloat16.shaderInt8 == VK_TRUE;
	if (instanceVersion >= VK_API_VERSION_1_3)
	{
		m_shaderDemoteToHelperInvocation = supported13.shaderDemoteToHelperInvocation == VK_TRUE;
		m_subgroupSizeControl = supported13.subgroupSizeControl == VK_TRUE;
		m_computeFullSubgroups = supported13.computeFullSubgroups == VK_TRUE;
		m_shaderIntegerDotProduct = supported13.shaderIntegerDotProduct == VK_TRUE;
	}

	VkPhysicalDevice16BitStorageFeatures storage16Query = {};
	VkPhysicalDeviceShaderDrawParametersFeatures drawParamQuery = {};
	VkPhysicalDevice8BitStorageFeatures storage8Query = {};
	VkPhysicalDeviceShaderAtomicInt64Features atomicInt64Query = {};
	VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures subgroupExtTypesQuery = {};
	VkPhysicalDeviceSubgroupSizeControlFeatures subgroupSizeQuery = {};
	VkPhysicalDeviceShaderIntegerDotProductFeatures integerDotQuery = {};
	VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures demoteQuery = {};
	VkPhysicalDeviceShaderSubgroupRotateFeatures subgroupRotateQuery = {};
	VkPhysicalDeviceShaderFloatControls2Features floatControls2Query = {};
	VkPhysicalDeviceShaderSubgroupPartitionedFeaturesEXT subgroupPartitionedQuery = {};
	VkPhysicalDeviceShaderQuadControlFeaturesKHR quadControlQuery = {};
	VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR uniformControlFlowQuery = {};
	VkPhysicalDeviceShaderMaximalReconvergenceFeaturesKHR maximalReconvergenceQuery = {};
	VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR computeDerivativesQuery = {};
	VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR barycentricQuery = {};
	VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT imageAtomicInt64Query = {};
	VkPhysicalDeviceShaderAtomicFloatFeaturesEXT shaderAtomicFloatQuery = {};
	VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT shaderAtomicFloat2Query = {};
	VkPhysicalDeviceShaderClockFeaturesKHR shaderClockQuery = {};
	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingQuery = {};
	VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutQuery = {};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureQuery = {};
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineQuery = {};
	VkPhysicalDeviceRayQueryFeaturesKHR rayQueryQuery = {};
	VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR rayTracingMaintenance1Query = {};
	VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR rayTracingPositionFetchQuery = {};
	VkPhysicalDeviceOpacityMicromapFeaturesKHR opacityMicromapQuery = {};
	VkPhysicalDeviceOpacityMicromapFeaturesEXT opacityMicromapExtQuery = {};
	VkPhysicalDeviceIndexTypeUint8Features indexTypeUint8Query = {};
	VkPhysicalDeviceRayTracingInvocationReorderFeaturesEXT invocationReorderQuery = {};
	VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesKHR pipelineLibraryGroupHandlesQuery = {};
	VkPhysicalDeviceCooperativeMatrixFeaturesKHR cooperativeMatrixQuery = {};
	VkPhysicalDeviceShaderBfloat16FeaturesKHR shaderBfloat16Query = {};
	VkPhysicalDeviceShaderFloat8FeaturesEXT shaderFloat8Query = {};
	VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR workgroupLayoutQuery = {};
	VkPhysicalDeviceShaderLongVectorFeaturesEXT longVectorQuery = {};
	VkPhysicalDeviceCooperativeMatrixMaintenance1FeaturesEXT cooperativeMatrixMaintenance1Query = {};
	void* shaderHead = nullptr;
	auto linkQuery = [&](void*& next, void* feature)
	{
		next = shaderHead;
		shaderHead = feature;
	};
	if (instanceVersion < VK_API_VERSION_1_1 && storage16Ext)
	{
		storage16Query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
		linkQuery(storage16Query.pNext, &storage16Query);
	}
	if (instanceVersion < VK_API_VERSION_1_1 && drawParamExt)
	{
		drawParamQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
		linkQuery(drawParamQuery.pNext, &drawParamQuery);
	}
	if (instanceVersion < VK_API_VERSION_1_2 && storage8Ext)
	{
		storage8Query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
		linkQuery(storage8Query.pNext, &storage8Query);
	}
	if (instanceVersion < VK_API_VERSION_1_2 && atomicInt64Ext)
	{
		atomicInt64Query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
		linkQuery(atomicInt64Query.pNext, &atomicInt64Query);
	}
	if (instanceVersion < VK_API_VERSION_1_2 && subgroupExtTypesExt)
	{
		subgroupExtTypesQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES;
		linkQuery(subgroupExtTypesQuery.pNext, &subgroupExtTypesQuery);
	}
	if (instanceVersion < VK_API_VERSION_1_3 && subgroupSizeExt)
	{
		subgroupSizeQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES;
		linkQuery(subgroupSizeQuery.pNext, &subgroupSizeQuery);
	}
	if (instanceVersion < VK_API_VERSION_1_3 && integerDotExt)
	{
		integerDotQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES;
		linkQuery(integerDotQuery.pNext, &integerDotQuery);
	}
	if (instanceVersion < VK_API_VERSION_1_3 && demoteExt)
	{
		demoteQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES;
		linkQuery(demoteQuery.pNext, &demoteQuery);
	}
	if (subgroupRotateExt)
	{
		subgroupRotateQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_ROTATE_FEATURES;
		linkQuery(subgroupRotateQuery.pNext, &subgroupRotateQuery);
	}
	if (floatControls2Ext)
	{
		floatControls2Query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT_CONTROLS_2_FEATURES;
		linkQuery(floatControls2Query.pNext, &floatControls2Query);
	}
	if (subgroupPartitionedExt)
	{
		subgroupPartitionedQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_PARTITIONED_FEATURES_EXT;
		linkQuery(subgroupPartitionedQuery.pNext, &subgroupPartitionedQuery);
	}
	if (quadControlExt)
	{
		quadControlQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_QUAD_CONTROL_FEATURES_KHR;
		linkQuery(quadControlQuery.pNext, &quadControlQuery);
	}
	if (uniformControlFlowExt)
	{
		uniformControlFlowQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR;
		linkQuery(uniformControlFlowQuery.pNext, &uniformControlFlowQuery);
	}
	if (maximalReconvergenceExt)
	{
		maximalReconvergenceQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MAXIMAL_RECONVERGENCE_FEATURES_KHR;
		linkQuery(maximalReconvergenceQuery.pNext, &maximalReconvergenceQuery);
	}
	if (computeDerivativesExt)
	{
		computeDerivativesQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_KHR;
		linkQuery(computeDerivativesQuery.pNext, &computeDerivativesQuery);
	}
	if (barycentricExt)
	{
		barycentricQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;
		linkQuery(barycentricQuery.pNext, &barycentricQuery);
	}
	if (imageAtomicInt64Ext)
	{
		imageAtomicInt64Query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT;
		linkQuery(imageAtomicInt64Query.pNext, &imageAtomicInt64Query);
	}
	if (instanceVersion < VK_API_VERSION_1_2 && descriptorIndexingExt)
	{
		descriptorIndexingQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		linkQuery(descriptorIndexingQuery.pNext, &descriptorIndexingQuery);
	}
	if (instanceVersion < VK_API_VERSION_1_2 && scalarBlockLayoutExt)
	{
		scalarBlockLayoutQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
		linkQuery(scalarBlockLayoutQuery.pNext, &scalarBlockLayoutQuery);
	}
	if (shaderAtomicFloatExt)
	{
		shaderAtomicFloatQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
		linkQuery(shaderAtomicFloatQuery.pNext, &shaderAtomicFloatQuery);
	}
	if (shaderAtomicFloat2Ext)
	{
		shaderAtomicFloat2Query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT;
		linkQuery(shaderAtomicFloat2Query.pNext, &shaderAtomicFloat2Query);
	}
	if (shaderClockExt)
	{
		shaderClockQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR;
		linkQuery(shaderClockQuery.pNext, &shaderClockQuery);
	}
	if (accelerationStructureExt)
	{
		accelerationStructureQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		linkQuery(accelerationStructureQuery.pNext, &accelerationStructureQuery);
	}
	if (rayTracingPipelineExt)
	{
		rayTracingPipelineQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		linkQuery(rayTracingPipelineQuery.pNext, &rayTracingPipelineQuery);
	}
	if (rayQueryExt)
	{
		rayQueryQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
		linkQuery(rayQueryQuery.pNext, &rayQueryQuery);
	}
	if (rayTracingMaintenance1Ext)
	{
		rayTracingMaintenance1Query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR;
		linkQuery(rayTracingMaintenance1Query.pNext, &rayTracingMaintenance1Query);
	}
	if (rayTracingPositionFetchExt)
	{
		rayTracingPositionFetchQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR;
		linkQuery(rayTracingPositionFetchQuery.pNext, &rayTracingPositionFetchQuery);
	}
	if (opacityMicromapKhr)
	{
		opacityMicromapQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_KHR;
		linkQuery(opacityMicromapQuery.pNext, &opacityMicromapQuery);
	}
	else if (opacityMicromapExtName)
	{
		opacityMicromapExtQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_EXT;
		linkQuery(opacityMicromapExtQuery.pNext, &opacityMicromapExtQuery);
	}
	if (indexTypeUint8Ext)
	{
		indexTypeUint8Query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES;
		linkQuery(indexTypeUint8Query.pNext, &indexTypeUint8Query);
	}
	if (invocationReorderExt)
	{
		invocationReorderQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_FEATURES_EXT;
		linkQuery(invocationReorderQuery.pNext, &invocationReorderQuery);
	}
	if (pipelineLibraryGroupHandlesExtName)
	{
		pipelineLibraryGroupHandlesQuery.sType =
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_LIBRARY_GROUP_HANDLES_FEATURES_KHR;
		linkQuery(pipelineLibraryGroupHandlesQuery.pNext, &pipelineLibraryGroupHandlesQuery);
	}
	if (cooperativeMatrixExt)
	{
		cooperativeMatrixQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR;
		linkQuery(cooperativeMatrixQuery.pNext, &cooperativeMatrixQuery);
	}
	if (shaderBfloat16Ext)
	{
		shaderBfloat16Query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_BFLOAT16_FEATURES_KHR;
		linkQuery(shaderBfloat16Query.pNext, &shaderBfloat16Query);
	}
	if (shaderFloat8Ext)
	{
		shaderFloat8Query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT8_FEATURES_EXT;
		linkQuery(shaderFloat8Query.pNext, &shaderFloat8Query);
	}
	if (workgroupLayoutExt)
	{
		workgroupLayoutQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR;
		linkQuery(workgroupLayoutQuery.pNext, &workgroupLayoutQuery);
	}
	if (longVectorExt)
	{
		longVectorQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_LONG_VECTOR_FEATURES_EXT;
		linkQuery(longVectorQuery.pNext, &longVectorQuery);
	}
	if (cooperativeMatrixMaintenance1Ext)
	{
		cooperativeMatrixMaintenance1Query.sType =
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_MAINTENANCE_1_FEATURES_EXT;
		linkQuery(cooperativeMatrixMaintenance1Query.pNext, &cooperativeMatrixMaintenance1Query);
	}
	if (shaderHead)
	{
		VkPhysicalDeviceFeatures2 shaderFeatures = {};
		shaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		shaderFeatures.pNext = shaderHead;
		vkGetPhysicalDeviceFeatures2(m_physicalDevice, &shaderFeatures);
	}
	if (instanceVersion < VK_API_VERSION_1_1 && storage16Ext)
	{
		m_storageBuffer16BitAccess = storage16Query.storageBuffer16BitAccess == VK_TRUE;
		m_uniformAndStorageBuffer16BitAccess = storage16Query.uniformAndStorageBuffer16BitAccess == VK_TRUE;
		m_storagePushConstant16 = storage16Query.storagePushConstant16 == VK_TRUE;
		m_storageInputOutput16 = storage16Query.storageInputOutput16 == VK_TRUE;
	}
	if (instanceVersion < VK_API_VERSION_1_1 && drawParamExt)
		m_shaderDrawParameters = drawParamQuery.shaderDrawParameters == VK_TRUE;
	if (instanceVersion < VK_API_VERSION_1_2 && storage8Ext)
	{
		m_storageBuffer8BitAccess = storage8Query.storageBuffer8BitAccess == VK_TRUE;
		m_uniformAndStorageBuffer8BitAccess = storage8Query.uniformAndStorageBuffer8BitAccess == VK_TRUE;
		m_storagePushConstant8 = storage8Query.storagePushConstant8 == VK_TRUE;
	}
	if (instanceVersion < VK_API_VERSION_1_2 && atomicInt64Ext)
	{
		m_shaderBufferInt64Atomics = atomicInt64Query.shaderBufferInt64Atomics == VK_TRUE;
		m_shaderSharedInt64Atomics = atomicInt64Query.shaderSharedInt64Atomics == VK_TRUE;
	}
	if (instanceVersion < VK_API_VERSION_1_2 && subgroupExtTypesExt)
		m_shaderSubgroupExtendedTypes = subgroupExtTypesQuery.shaderSubgroupExtendedTypes == VK_TRUE;
	if (instanceVersion < VK_API_VERSION_1_3 && subgroupSizeExt)
	{
		m_subgroupSizeControl = subgroupSizeQuery.subgroupSizeControl == VK_TRUE;
		m_computeFullSubgroups = subgroupSizeQuery.computeFullSubgroups == VK_TRUE;
	}
	if (instanceVersion < VK_API_VERSION_1_3 && integerDotExt)
		m_shaderIntegerDotProduct = integerDotQuery.shaderIntegerDotProduct == VK_TRUE;
	if (instanceVersion < VK_API_VERSION_1_3 && demoteExt)
		m_shaderDemoteToHelperInvocation = demoteQuery.shaderDemoteToHelperInvocation == VK_TRUE;
	if (subgroupRotateExt)
	{
		m_shaderSubgroupRotate = subgroupRotateQuery.shaderSubgroupRotate == VK_TRUE;
		m_shaderSubgroupRotateClustered = subgroupRotateQuery.shaderSubgroupRotateClustered == VK_TRUE;
	}
	if (floatControls2Ext) m_shaderFloatControls2 = floatControls2Query.shaderFloatControls2 == VK_TRUE;
	if (subgroupPartitionedExt)
		m_shaderSubgroupPartitioned = subgroupPartitionedQuery.shaderSubgroupPartitioned == VK_TRUE;
	if (quadControlExt) m_shaderQuadControl = quadControlQuery.shaderQuadControl == VK_TRUE;
	if (uniformControlFlowExt)
		m_shaderSubgroupUniformControlFlow = uniformControlFlowQuery.shaderSubgroupUniformControlFlow == VK_TRUE;
	if (maximalReconvergenceExt)
		m_shaderMaximalReconvergence = maximalReconvergenceQuery.shaderMaximalReconvergence == VK_TRUE;
	if (computeDerivativesExt)
	{
		m_computeDerivativeGroupQuads = computeDerivativesQuery.computeDerivativeGroupQuads == VK_TRUE;
		m_computeDerivativeGroupLinear = computeDerivativesQuery.computeDerivativeGroupLinear == VK_TRUE;
	}
	if (barycentricExt) m_fragmentShaderBarycentric = barycentricQuery.fragmentShaderBarycentric == VK_TRUE;
	if (imageAtomicInt64Ext)
	{
		m_shaderImageInt64Atomics = imageAtomicInt64Query.shaderImageInt64Atomics == VK_TRUE;
		m_sparseImageInt64Atomics = imageAtomicInt64Query.sparseImageInt64Atomics == VK_TRUE;
	}
	if (shaderAtomicFloatExt)
	{
		m_shaderBufferFloat32Atomics = shaderAtomicFloatQuery.shaderBufferFloat32Atomics == VK_TRUE;
		m_shaderBufferFloat32AtomicAdd = shaderAtomicFloatQuery.shaderBufferFloat32AtomicAdd == VK_TRUE;
		m_shaderBufferFloat64Atomics = shaderAtomicFloatQuery.shaderBufferFloat64Atomics == VK_TRUE;
		m_shaderBufferFloat64AtomicAdd = shaderAtomicFloatQuery.shaderBufferFloat64AtomicAdd == VK_TRUE;
		m_shaderSharedFloat32Atomics = shaderAtomicFloatQuery.shaderSharedFloat32Atomics == VK_TRUE;
		m_shaderSharedFloat32AtomicAdd = shaderAtomicFloatQuery.shaderSharedFloat32AtomicAdd == VK_TRUE;
		m_shaderSharedFloat64Atomics = shaderAtomicFloatQuery.shaderSharedFloat64Atomics == VK_TRUE;
		m_shaderSharedFloat64AtomicAdd = shaderAtomicFloatQuery.shaderSharedFloat64AtomicAdd == VK_TRUE;
		m_shaderImageFloat32Atomics = shaderAtomicFloatQuery.shaderImageFloat32Atomics == VK_TRUE;
		m_shaderImageFloat32AtomicAdd = shaderAtomicFloatQuery.shaderImageFloat32AtomicAdd == VK_TRUE;
		m_sparseImageFloat32Atomics = shaderAtomicFloatQuery.sparseImageFloat32Atomics == VK_TRUE;
		m_sparseImageFloat32AtomicAdd = shaderAtomicFloatQuery.sparseImageFloat32AtomicAdd == VK_TRUE;
	}
	if (shaderAtomicFloat2Ext)
	{
		m_shaderBufferFloat16Atomics = shaderAtomicFloat2Query.shaderBufferFloat16Atomics == VK_TRUE;
		m_shaderBufferFloat16AtomicAdd = shaderAtomicFloat2Query.shaderBufferFloat16AtomicAdd == VK_TRUE;
		m_shaderBufferFloat16AtomicMinMax = shaderAtomicFloat2Query.shaderBufferFloat16AtomicMinMax == VK_TRUE;
		m_shaderBufferFloat32AtomicMinMax = shaderAtomicFloat2Query.shaderBufferFloat32AtomicMinMax == VK_TRUE;
		m_shaderBufferFloat64AtomicMinMax = shaderAtomicFloat2Query.shaderBufferFloat64AtomicMinMax == VK_TRUE;
		m_shaderSharedFloat16Atomics = shaderAtomicFloat2Query.shaderSharedFloat16Atomics == VK_TRUE;
		m_shaderSharedFloat16AtomicAdd = shaderAtomicFloat2Query.shaderSharedFloat16AtomicAdd == VK_TRUE;
		m_shaderSharedFloat16AtomicMinMax = shaderAtomicFloat2Query.shaderSharedFloat16AtomicMinMax == VK_TRUE;
		m_shaderSharedFloat32AtomicMinMax = shaderAtomicFloat2Query.shaderSharedFloat32AtomicMinMax == VK_TRUE;
		m_shaderSharedFloat64AtomicMinMax = shaderAtomicFloat2Query.shaderSharedFloat64AtomicMinMax == VK_TRUE;
		m_shaderImageFloat32AtomicMinMax = shaderAtomicFloat2Query.shaderImageFloat32AtomicMinMax == VK_TRUE;
		m_sparseImageFloat32AtomicMinMax = shaderAtomicFloat2Query.sparseImageFloat32AtomicMinMax == VK_TRUE;
	}
	if (shaderClockExt)
	{
		m_shaderSubgroupClock = shaderClockQuery.shaderSubgroupClock == VK_TRUE;
		m_shaderDeviceClock = shaderClockQuery.shaderDeviceClock == VK_TRUE;
	}
	if (!m_shaderBufferFloat32Atomics) m_shaderBufferFloat32AtomicAdd = false;
	if (!m_shaderSharedFloat32Atomics) m_shaderSharedFloat32AtomicAdd = false;
	if (!m_shaderImageFloat32Atomics)
	{
		m_shaderImageFloat32AtomicAdd = false;
		m_sparseImageFloat32Atomics = false;
	}
	if (!m_sparseImageFloat32Atomics) m_sparseImageFloat32AtomicAdd = false;
	if (features.features.sparseBinding != VK_TRUE)
	{
		m_sparseImageFloat32Atomics = false;
		m_sparseImageFloat32AtomicAdd = false;
	}
	if (!m_shaderFloat64)
	{
		m_shaderBufferFloat64Atomics = false;
		m_shaderBufferFloat64AtomicAdd = false;
		m_shaderSharedFloat64Atomics = false;
		m_shaderSharedFloat64AtomicAdd = false;
	}
	if (!m_shaderBufferFloat64Atomics) m_shaderBufferFloat64AtomicAdd = false;
	if (!m_shaderSharedFloat64Atomics) m_shaderSharedFloat64AtomicAdd = false;
	if (!m_storageBuffer16BitAccess)
	{
		m_shaderBufferFloat16Atomics = false;
		m_shaderBufferFloat16AtomicAdd = false;
		m_shaderBufferFloat16AtomicMinMax = false;
	}
	if (!m_shaderBufferFloat16Atomics)
	{
		m_shaderBufferFloat16AtomicAdd = false;
		m_shaderBufferFloat16AtomicMinMax = false;
	}
	if (!m_shaderBufferFloat32Atomics) m_shaderBufferFloat32AtomicMinMax = false;
	if (!m_shaderBufferFloat64Atomics) m_shaderBufferFloat64AtomicMinMax = false;
	if (!m_shaderSharedFloat16Atomics)
	{
		m_shaderSharedFloat16AtomicAdd = false;
		m_shaderSharedFloat16AtomicMinMax = false;
	}
	if (!m_shaderSharedFloat32Atomics) m_shaderSharedFloat32AtomicMinMax = false;
	if (!m_shaderSharedFloat64Atomics) m_shaderSharedFloat64AtomicMinMax = false;
	if (!m_shaderImageFloat32Atomics) m_shaderImageFloat32AtomicMinMax = false;
	if (!m_sparseImageFloat32Atomics || !m_shaderImageFloat32AtomicMinMax)
		m_sparseImageFloat32AtomicMinMax = false;
	if (instanceVersion < VK_API_VERSION_1_2 && descriptorIndexingExt)
	{
		m_shaderInputAttachmentArrayDynamicIndexing = descriptorIndexingQuery.shaderInputAttachmentArrayDynamicIndexing == VK_TRUE;
		m_shaderUniformTexelBufferArrayDynamicIndexing = descriptorIndexingQuery.shaderUniformTexelBufferArrayDynamicIndexing == VK_TRUE;
		m_shaderStorageTexelBufferArrayDynamicIndexing = descriptorIndexingQuery.shaderStorageTexelBufferArrayDynamicIndexing == VK_TRUE;
		m_shaderUniformBufferArrayNonUniformIndexing = descriptorIndexingQuery.shaderUniformBufferArrayNonUniformIndexing == VK_TRUE;
		m_shaderSampledImageArrayNonUniformIndexing = descriptorIndexingQuery.shaderSampledImageArrayNonUniformIndexing == VK_TRUE;
		m_shaderStorageBufferArrayNonUniformIndexing = descriptorIndexingQuery.shaderStorageBufferArrayNonUniformIndexing == VK_TRUE;
		m_shaderStorageImageArrayNonUniformIndexing = descriptorIndexingQuery.shaderStorageImageArrayNonUniformIndexing == VK_TRUE;
		m_shaderInputAttachmentArrayNonUniformIndexing = descriptorIndexingQuery.shaderInputAttachmentArrayNonUniformIndexing == VK_TRUE;
		m_shaderUniformTexelBufferArrayNonUniformIndexing = descriptorIndexingQuery.shaderUniformTexelBufferArrayNonUniformIndexing == VK_TRUE;
		m_shaderStorageTexelBufferArrayNonUniformIndexing = descriptorIndexingQuery.shaderStorageTexelBufferArrayNonUniformIndexing == VK_TRUE;
		m_runtimeDescriptorArray = descriptorIndexingQuery.runtimeDescriptorArray == VK_TRUE;
		m_descriptorBindingPartiallyBound = descriptorIndexingQuery.descriptorBindingPartiallyBound == VK_TRUE;
	}
	if (instanceVersion < VK_API_VERSION_1_2 && scalarBlockLayoutExt)
		m_scalarBlockLayout = scalarBlockLayoutQuery.scalarBlockLayout == VK_TRUE;
	if (instanceVersion < VK_API_VERSION_1_2 && viewportIndexLayerExt)
	{
		m_shaderOutputViewportIndex = true;
		m_shaderOutputLayer = true;
	}
	if (!m_shaderUniformBufferArrayDynamicIndexing) m_shaderUniformBufferArrayNonUniformIndexing = false;
	if (!m_shaderSampledImageArrayDynamicIndexing) m_shaderSampledImageArrayNonUniformIndexing = false;
	if (!m_shaderStorageBufferArrayDynamicIndexing) m_shaderStorageBufferArrayNonUniformIndexing = false;
	if (!m_shaderStorageImageArrayDynamicIndexing) m_shaderStorageImageArrayNonUniformIndexing = false;
	if (!m_shaderInputAttachmentArrayDynamicIndexing) m_shaderInputAttachmentArrayNonUniformIndexing = false;
	if (!m_shaderUniformTexelBufferArrayDynamicIndexing) m_shaderUniformTexelBufferArrayNonUniformIndexing = false;
	if (!m_shaderStorageTexelBufferArrayDynamicIndexing) m_shaderStorageTexelBufferArrayNonUniformIndexing = false;
	if (!m_shaderSampledImageArrayNonUniformIndexing) m_runtimeDescriptorArray = false;
	if (!coreDynamicRendering && !subgroupSizeExt)
	{
		m_subgroupSizeControl = false;
		m_computeFullSubgroups = false;
	}
	if (!coreDynamicRendering && !integerDotExt) m_shaderIntegerDotProduct = false;
	if (!coreDynamicRendering && !demoteExt) m_shaderDemoteToHelperInvocation = false;
	if (!m_storageBuffer16BitAccess)
	{
		m_uniformAndStorageBuffer16BitAccess = false;
		m_storagePushConstant16 = false;
		m_storageInputOutput16 = false;
	}
	if (!m_storageBuffer8BitAccess)
	{
		m_uniformAndStorageBuffer8BitAccess = false;
		m_storagePushConstant8 = false;
	}
	if (!m_shaderSubgroupRotate) m_shaderSubgroupRotateClustered = false;
	if (!m_subgroupSizeControl) m_computeFullSubgroups = false;
	if (!m_shaderInt64)
	{
		m_shaderBufferInt64Atomics = false;
		m_shaderSharedInt64Atomics = false;
		m_shaderImageInt64Atomics = false;
		m_sparseImageInt64Atomics = false;
	}
	if (!m_shaderImageInt64Atomics) m_sparseImageInt64Atomics = false;
	m_shaderSubgroupBallot = !core11 && HasExtension(exts, VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME);
	m_shaderSubgroupVote = !core11 && HasExtension(exts, VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME);

	const auto storage16On = m_storageBuffer16BitAccess || m_uniformAndStorageBuffer16BitAccess ||
		m_storagePushConstant16 || m_storageInputOutput16;
	const auto storage8On = m_storageBuffer8BitAccess || m_uniformAndStorageBuffer8BitAccess || m_storagePushConstant8;
	if (!core11 && storage16Ext && storage16On) enabled.push_back(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
	if (!core11 && drawParamExt && m_shaderDrawParameters)
		enabled.push_back(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);
	if (!core12 && storage8Ext && storage8On) enabled.push_back(VK_KHR_8BIT_STORAGE_EXTENSION_NAME);
	if (!core12 && atomicInt64Ext && (m_shaderBufferInt64Atomics || m_shaderSharedInt64Atomics))
		enabled.push_back(VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME);
	if (!core12 && subgroupExtTypesExt && m_shaderSubgroupExtendedTypes)
		enabled.push_back(VK_KHR_SHADER_SUBGROUP_EXTENDED_TYPES_EXTENSION_NAME);
	if (!coreDynamicRendering && subgroupSizeExt && (m_subgroupSizeControl || m_computeFullSubgroups))
		enabled.push_back(VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);
	if (!coreDynamicRendering && integerDotExt && m_shaderIntegerDotProduct)
		enabled.push_back(VK_KHR_SHADER_INTEGER_DOT_PRODUCT_EXTENSION_NAME);
	if (!coreDynamicRendering && demoteExt && m_shaderDemoteToHelperInvocation)
		enabled.push_back(VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME);
	if (subgroupRotateExt && (m_shaderSubgroupRotate || m_shaderSubgroupRotateClustered))
		enabled.push_back(VK_KHR_SHADER_SUBGROUP_ROTATE_EXTENSION_NAME);
	if (floatControls2Ext && m_shaderFloatControls2)
		enabled.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_2_EXTENSION_NAME);
	if (subgroupPartitionedExt && m_shaderSubgroupPartitioned)
		enabled.push_back(VK_EXT_SHADER_SUBGROUP_PARTITIONED_EXTENSION_NAME);
	if (quadControlExt && m_shaderQuadControl) enabled.push_back(VK_KHR_SHADER_QUAD_CONTROL_EXTENSION_NAME);
	if (uniformControlFlowExt && m_shaderSubgroupUniformControlFlow)
		enabled.push_back(VK_KHR_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_EXTENSION_NAME);
	if (maximalReconvergenceExt && m_shaderMaximalReconvergence)
		enabled.push_back(VK_KHR_SHADER_MAXIMAL_RECONVERGENCE_EXTENSION_NAME);
	if (computeDerivativesExt && (m_computeDerivativeGroupQuads || m_computeDerivativeGroupLinear))
		enabled.push_back(VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME);
	if (barycentricExt && m_fragmentShaderBarycentric)
		enabled.push_back(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
	if (imageAtomicInt64Ext && (m_shaderImageInt64Atomics || m_sparseImageInt64Atomics))
		enabled.push_back(VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME);
	const auto shaderAtomicFloatOn = m_shaderBufferFloat32Atomics || m_shaderBufferFloat32AtomicAdd ||
		m_shaderBufferFloat64Atomics || m_shaderBufferFloat64AtomicAdd ||
		m_shaderSharedFloat32Atomics || m_shaderSharedFloat32AtomicAdd ||
		m_shaderSharedFloat64Atomics || m_shaderSharedFloat64AtomicAdd ||
		m_shaderImageFloat32Atomics || m_shaderImageFloat32AtomicAdd ||
		m_sparseImageFloat32Atomics || m_sparseImageFloat32AtomicAdd;
	const auto shaderAtomicFloat2On = m_shaderBufferFloat16Atomics || m_shaderBufferFloat16AtomicAdd ||
		m_shaderBufferFloat16AtomicMinMax || m_shaderBufferFloat32AtomicMinMax || m_shaderBufferFloat64AtomicMinMax ||
		m_shaderSharedFloat16Atomics || m_shaderSharedFloat16AtomicAdd || m_shaderSharedFloat16AtomicMinMax ||
		m_shaderSharedFloat32AtomicMinMax || m_shaderSharedFloat64AtomicMinMax ||
		m_shaderImageFloat32AtomicMinMax || m_sparseImageFloat32AtomicMinMax;
	if (shaderAtomicFloatExt && (shaderAtomicFloatOn || shaderAtomicFloat2On))
		enabled.push_back(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME);
	if (shaderAtomicFloat2Ext && shaderAtomicFloat2On)
		enabled.push_back(VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME);
	if (shaderClockExt && (m_shaderSubgroupClock || m_shaderDeviceClock))
		enabled.push_back(VK_KHR_SHADER_CLOCK_EXTENSION_NAME);
	m_imageFormatList = core12 || HasExtension(exts, VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
	if (!core12 && m_imageFormatList) enabled.push_back(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
	if (m_hlslFunctionality1) enabled.push_back(VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME);
	if (m_shaderSubgroupBallot) enabled.push_back(VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME);
	if (m_shaderSubgroupVote) enabled.push_back(VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME);
	if (accelerationStructureExt)
	{
		m_accelerationStructure = accelerationStructureQuery.accelerationStructure == VK_TRUE;
		m_accelerationStructureCaptureReplay = accelerationStructureQuery.accelerationStructureCaptureReplay == VK_TRUE;
		m_accelerationStructureIndirectBuild = accelerationStructureQuery.accelerationStructureIndirectBuild == VK_TRUE;
		m_accelerationStructureHostCommands = accelerationStructureQuery.accelerationStructureHostCommands == VK_TRUE;
	}
	if (!core12 && !descriptorIndexingExt) m_accelerationStructure = false;
	if (!m_accelerationStructure)
	{
		m_accelerationStructureCaptureReplay = false;
		m_accelerationStructureIndirectBuild = false;
		m_accelerationStructureHostCommands = false;
	}
	if (rayTracingPipelineExt && m_accelerationStructure && m_spirv14 && m_shaderFloatControls)
	{
		m_rayTracingPipeline = rayTracingPipelineQuery.rayTracingPipeline == VK_TRUE;
		m_rayTracingPipelineShaderGroupHandleCaptureReplay =
			rayTracingPipelineQuery.rayTracingPipelineShaderGroupHandleCaptureReplay == VK_TRUE;
		m_rayTracingPipelineShaderGroupHandleCaptureReplayMixed =
			rayTracingPipelineQuery.rayTracingPipelineShaderGroupHandleCaptureReplayMixed == VK_TRUE;
		m_rayTracingPipelineTraceRaysIndirect = rayTracingPipelineQuery.rayTracingPipelineTraceRaysIndirect == VK_TRUE;
		m_rayTraversalPrimitiveCulling = rayTracingPipelineQuery.rayTraversalPrimitiveCulling == VK_TRUE;
	}
	if (!m_rayTracingPipeline)
	{
		m_rayTracingPipelineShaderGroupHandleCaptureReplay = false;
		m_rayTracingPipelineShaderGroupHandleCaptureReplayMixed = false;
		m_rayTracingPipelineTraceRaysIndirect = false;
		m_rayTraversalPrimitiveCulling = false;
	}
	if (!m_rayTracingPipelineShaderGroupHandleCaptureReplay)
		m_rayTracingPipelineShaderGroupHandleCaptureReplayMixed = false;
	if (m_rayTracingPipeline)
	{
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProps = {};
		rayTracingProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		VkPhysicalDeviceProperties2 rayTracingProps2 = {};
		rayTracingProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		rayTracingProps2.pNext = &rayTracingProps;
		vkGetPhysicalDeviceProperties2(m_physicalDevice, &rayTracingProps2);
		if (rayTracingProps.shaderGroupHandleSize) m_rtPipelineProps = rayTracingProps;
		if (!m_rtPipelineProps.shaderGroupHandleSize) m_rtPipelineProps.shaderGroupHandleSize = 32;
		if (!m_rtPipelineProps.shaderGroupHandleAlignment) m_rtPipelineProps.shaderGroupHandleAlignment = 32;
		if (!m_rtPipelineProps.shaderGroupBaseAlignment) m_rtPipelineProps.shaderGroupBaseAlignment = 64;
		if (!m_rtPipelineProps.maxRayRecursionDepth) m_rtPipelineProps.maxRayRecursionDepth = 1;
		m_rtPipelineProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		m_rtPipelineProps.pNext = nullptr;
	}
	if (rayQueryExt && m_accelerationStructure && m_spirv14 && m_shaderFloatControls)
		m_rayQuery = rayQueryQuery.rayQuery == VK_TRUE;
	if (rayTracingMaintenance1Ext && m_accelerationStructure)
	{
		m_rayTracingMaintenance1 = rayTracingMaintenance1Query.rayTracingMaintenance1 == VK_TRUE;
		m_rayTracingPipelineTraceRaysIndirect2 =
			rayTracingMaintenance1Query.rayTracingPipelineTraceRaysIndirect2 == VK_TRUE;
	}
	if (!m_rayTracingMaintenance1 || !m_rayTracingPipelineTraceRaysIndirect)
		m_rayTracingPipelineTraceRaysIndirect2 = false;
	if (rayTracingPositionFetchExt && m_accelerationStructure)
		m_rayTracingPositionFetch = rayTracingPositionFetchQuery.rayTracingPositionFetch == VK_TRUE;
	if (opacityMicromapExtName && m_accelerationStructure && m_synchronization2)
	{
		if (opacityMicromapKhr)
		{
			m_micromap = opacityMicromapQuery.micromap == VK_TRUE;
			m_micromapKhr = m_micromap;
		}
		else
		{
			m_micromap = opacityMicromapExtQuery.micromap == VK_TRUE;
			m_micromapCaptureReplay = opacityMicromapExtQuery.micromapCaptureReplay == VK_TRUE;
			m_micromapHostCommands = opacityMicromapExtQuery.micromapHostCommands == VK_TRUE;
		}
	}
	if (!m_micromap)
	{
		m_micromapKhr = false;
		m_micromapCaptureReplay = false;
		m_micromapHostCommands = false;
	}
	if (indexTypeUint8Ext) m_indexTypeUint8 = indexTypeUint8Query.indexTypeUint8 == VK_TRUE;
	if (!m_deferredHostOperations) m_micromapHostCommands = false;
	if (invocationReorderExt && m_rayTracingPipeline)
		m_rayTracingInvocationReorder = invocationReorderQuery.rayTracingInvocationReorder == VK_TRUE;
	if (pipelineLibraryGroupHandlesExtName && m_pipelineLibrary && m_rayTracingPipeline)
		m_pipelineLibraryGroupHandles = pipelineLibraryGroupHandlesQuery.pipelineLibraryGroupHandles == VK_TRUE;
	const auto descriptorIndexingOn = m_shaderInputAttachmentArrayDynamicIndexing ||
		m_shaderUniformTexelBufferArrayDynamicIndexing || m_shaderStorageTexelBufferArrayDynamicIndexing ||
		m_shaderUniformBufferArrayNonUniformIndexing || m_shaderSampledImageArrayNonUniformIndexing ||
		m_shaderStorageBufferArrayNonUniformIndexing || m_shaderStorageImageArrayNonUniformIndexing ||
		m_shaderInputAttachmentArrayNonUniformIndexing || m_shaderUniformTexelBufferArrayNonUniformIndexing ||
		m_shaderStorageTexelBufferArrayNonUniformIndexing || m_runtimeDescriptorArray ||
		m_descriptorBindingPartiallyBound;
	const auto descriptorIndexingNeeded = descriptorIndexingOn || m_accelerationStructure;
	if (!core12 && descriptorIndexingExt && descriptorIndexingNeeded)
		enabled.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	if (!core12 && scalarBlockLayoutExt && m_scalarBlockLayout)
		enabled.push_back(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
	if (!core12 && viewportIndexLayerExt && (m_shaderOutputViewportIndex || m_shaderOutputLayer))
		enabled.push_back(VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME);
	if (!core12 && m_shaderFloatControls) enabled.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	if (!core12 && m_spirv14) enabled.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	if (m_deferredHostOperations) enabled.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	if (m_pipelineLibrary) enabled.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
	if (m_accelerationStructure) enabled.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	if (m_rayTracingPipeline) enabled.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
	if (m_rayQuery) enabled.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
	if (m_rayTracingMaintenance1 || m_rayTracingPipelineTraceRaysIndirect2)
		enabled.push_back(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME);
	if (m_rayTracingPositionFetch) enabled.push_back(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME);
	if (m_micromap) enabled.push_back(opacityMicromapExtName);
	if (m_indexTypeUint8) enabled.push_back(VK_KHR_INDEX_TYPE_UINT8_EXTENSION_NAME);
	if (m_rayTracingInvocationReorder)
		enabled.push_back(VK_EXT_RAY_TRACING_INVOCATION_REORDER_EXTENSION_NAME);
	if (m_pipelineLibraryGroupHandles) enabled.push_back(pipelineLibraryGroupHandlesExtName);
	if (cooperativeMatrixExt)
	{
		m_cooperativeMatrix = cooperativeMatrixQuery.cooperativeMatrix == VK_TRUE;
		m_cooperativeMatrixRobustBufferAccess = cooperativeMatrixQuery.cooperativeMatrixRobustBufferAccess == VK_TRUE;
	}
	if (!m_cooperativeMatrix || features.features.robustBufferAccess != VK_TRUE)
		m_cooperativeMatrixRobustBufferAccess = false;
	if (shaderBfloat16Ext)
	{
		m_shaderBFloat16Type = shaderBfloat16Query.shaderBFloat16Type == VK_TRUE;
		m_shaderBFloat16DotProduct = shaderBfloat16Query.shaderBFloat16DotProduct == VK_TRUE;
		m_shaderBFloat16CooperativeMatrix = shaderBfloat16Query.shaderBFloat16CooperativeMatrix == VK_TRUE;
	}
	if (!m_shaderBFloat16Type || !m_shaderIntegerDotProduct) m_shaderBFloat16DotProduct = false;
	if (!m_shaderBFloat16Type || !m_cooperativeMatrix) m_shaderBFloat16CooperativeMatrix = false;
	if (shaderFloat8Ext)
	{
		m_shaderFloat8 = shaderFloat8Query.shaderFloat8 == VK_TRUE;
		m_shaderFloat8CooperativeMatrix = shaderFloat8Query.shaderFloat8CooperativeMatrix == VK_TRUE;
	}
	if (!m_shaderFloat8 || !m_cooperativeMatrix) m_shaderFloat8CooperativeMatrix = false;
	if (workgroupLayoutExt)
	{
		m_workgroupMemoryExplicitLayout = workgroupLayoutQuery.workgroupMemoryExplicitLayout == VK_TRUE;
		m_workgroupMemoryExplicitLayoutScalarBlockLayout =
			workgroupLayoutQuery.workgroupMemoryExplicitLayoutScalarBlockLayout == VK_TRUE;
		m_workgroupMemoryExplicitLayout8BitAccess = workgroupLayoutQuery.workgroupMemoryExplicitLayout8BitAccess == VK_TRUE;
		m_workgroupMemoryExplicitLayout16BitAccess = workgroupLayoutQuery.workgroupMemoryExplicitLayout16BitAccess == VK_TRUE;
	}
	if (!m_workgroupMemoryExplicitLayout)
	{
		m_workgroupMemoryExplicitLayoutScalarBlockLayout = false;
		m_workgroupMemoryExplicitLayout8BitAccess = false;
		m_workgroupMemoryExplicitLayout16BitAccess = false;
	}
	if (!m_scalarBlockLayout) m_workgroupMemoryExplicitLayoutScalarBlockLayout = false;
	if (!m_storageBuffer8BitAccess) m_workgroupMemoryExplicitLayout8BitAccess = false;
	if (!m_storageBuffer16BitAccess) m_workgroupMemoryExplicitLayout16BitAccess = false;
	if (longVectorExt) m_longVector = longVectorQuery.longVector == VK_TRUE;
	if (m_longVector) m_maxLongVectorComponents = longVectorProps.maxVectorComponents;
	if (cooperativeMatrixMaintenance1Ext && m_cooperativeMatrix)
	{
		m_cooperativeMatrixProperties2 = cooperativeMatrixMaintenance1Query.cooperativeMatrixProperties2 == VK_TRUE;
		m_cooperativeMatrixReductions = cooperativeMatrixMaintenance1Query.cooperativeMatrixReductions == VK_TRUE;
		m_cooperativeMatrixConversions = cooperativeMatrixMaintenance1Query.cooperativeMatrixConversions == VK_TRUE;
		m_cooperativeMatrixPerElementOperations =
			cooperativeMatrixMaintenance1Query.cooperativeMatrixPerElementOperations == VK_TRUE;
		m_cooperativeMatrixGetCoordinate = cooperativeMatrixMaintenance1Query.cooperativeMatrixGetCoordinate == VK_TRUE;
	}
	const auto cooperativeMatrixMaintenance1On = m_cooperativeMatrixProperties2 || m_cooperativeMatrixReductions ||
		m_cooperativeMatrixConversions || m_cooperativeMatrixPerElementOperations || m_cooperativeMatrixGetCoordinate;
	const auto workgroupLayoutOn = m_workgroupMemoryExplicitLayout ||
		m_workgroupMemoryExplicitLayoutScalarBlockLayout || m_workgroupMemoryExplicitLayout8BitAccess ||
		m_workgroupMemoryExplicitLayout16BitAccess;
	if (m_cooperativeMatrix) enabled.push_back(VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME);
	if (m_shaderBFloat16Type || m_shaderBFloat16DotProduct || m_shaderBFloat16CooperativeMatrix)
		enabled.push_back(VK_KHR_SHADER_BFLOAT16_EXTENSION_NAME);
	if (m_shaderFloat8 || m_shaderFloat8CooperativeMatrix) enabled.push_back(VK_EXT_SHADER_FLOAT8_EXTENSION_NAME);
	if (workgroupLayoutOn) enabled.push_back(VK_KHR_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_EXTENSION_NAME);
	if (m_longVector) enabled.push_back(VK_EXT_SHADER_LONG_VECTOR_EXTENSION_NAME);
	if (cooperativeMatrixMaintenance1On)
		enabled.push_back(VK_EXT_COOPERATIVE_MATRIX_MAINTENANCE_1_EXTENSION_NAME);

	VkPhysicalDeviceVulkan12Features features12 = {};
	VkPhysicalDeviceBufferDeviceAddressFeatures addressEnable = {};
	VkPhysicalDeviceTimelineSemaphoreFeatures timelineEnable = {};
	VkPhysicalDeviceShaderFloat16Int8Features float16Enable = {};
	void* promoted = enableBase;
	if (instanceVersion >= VK_API_VERSION_1_2)
	{
		features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		features12.pNext = enableBase;
		features12.bufferDeviceAddress = VK_TRUE;
		features12.timelineSemaphore = VK_TRUE;
		features12.shaderFloat16 = VK_TRUE;
		features12.shaderInt8 = m_shaderInt8 ? VK_TRUE : VK_FALSE;
		features12.storageBuffer8BitAccess = m_storageBuffer8BitAccess ? VK_TRUE : VK_FALSE;
		features12.uniformAndStorageBuffer8BitAccess = m_uniformAndStorageBuffer8BitAccess ? VK_TRUE : VK_FALSE;
		features12.storagePushConstant8 = m_storagePushConstant8 ? VK_TRUE : VK_FALSE;
		features12.shaderBufferInt64Atomics = m_shaderBufferInt64Atomics ? VK_TRUE : VK_FALSE;
		features12.shaderSharedInt64Atomics = m_shaderSharedInt64Atomics ? VK_TRUE : VK_FALSE;
		features12.shaderSubgroupExtendedTypes = m_shaderSubgroupExtendedTypes ? VK_TRUE : VK_FALSE;
		features12.descriptorIndexing = descriptorIndexingNeeded ? VK_TRUE : VK_FALSE;
		features12.shaderInputAttachmentArrayDynamicIndexing = m_shaderInputAttachmentArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
		features12.shaderUniformTexelBufferArrayDynamicIndexing = m_shaderUniformTexelBufferArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
		features12.shaderStorageTexelBufferArrayDynamicIndexing = m_shaderStorageTexelBufferArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
		features12.shaderUniformBufferArrayNonUniformIndexing = m_shaderUniformBufferArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		features12.shaderSampledImageArrayNonUniformIndexing = m_shaderSampledImageArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		features12.shaderStorageBufferArrayNonUniformIndexing = m_shaderStorageBufferArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		features12.shaderStorageImageArrayNonUniformIndexing = m_shaderStorageImageArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		features12.shaderInputAttachmentArrayNonUniformIndexing = m_shaderInputAttachmentArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		features12.shaderUniformTexelBufferArrayNonUniformIndexing = m_shaderUniformTexelBufferArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		features12.shaderStorageTexelBufferArrayNonUniformIndexing = m_shaderStorageTexelBufferArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		features12.runtimeDescriptorArray = m_runtimeDescriptorArray ? VK_TRUE : VK_FALSE;
		features12.descriptorBindingPartiallyBound = m_descriptorBindingPartiallyBound ? VK_TRUE : VK_FALSE;
		features12.scalarBlockLayout = m_scalarBlockLayout ? VK_TRUE : VK_FALSE;
		features12.shaderOutputViewportIndex = m_shaderOutputViewportIndex ? VK_TRUE : VK_FALSE;
		features12.shaderOutputLayer = m_shaderOutputLayer ? VK_TRUE : VK_FALSE;
		features12.subgroupBroadcastDynamicId = m_subgroupBroadcastDynamicId ? VK_TRUE : VK_FALSE;
		promoted = &features12;
	}
	else
	{
		addressEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		addressEnable.pNext = enableBase;
		addressEnable.bufferDeviceAddress = VK_TRUE;
		timelineEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
		timelineEnable.pNext = &addressEnable;
		timelineEnable.timelineSemaphore = VK_TRUE;
		float16Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;
		float16Enable.pNext = &timelineEnable;
		float16Enable.shaderFloat16 = VK_TRUE;
		float16Enable.shaderInt8 = m_shaderInt8 ? VK_TRUE : VK_FALSE;
		promoted = &float16Enable;
	}

	VkPhysicalDeviceVulkan11Features features11 = {};
	if (instanceVersion >= VK_API_VERSION_1_1 && (storage16On || m_shaderDrawParameters))
	{
		features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		features11.pNext = enableBase;
		features11.storageBuffer16BitAccess = m_storageBuffer16BitAccess ? VK_TRUE : VK_FALSE;
		features11.uniformAndStorageBuffer16BitAccess = m_uniformAndStorageBuffer16BitAccess ? VK_TRUE : VK_FALSE;
		features11.storagePushConstant16 = m_storagePushConstant16 ? VK_TRUE : VK_FALSE;
		features11.storageInputOutput16 = m_storageInputOutput16 ? VK_TRUE : VK_FALSE;
		features11.shaderDrawParameters = m_shaderDrawParameters ? VK_TRUE : VK_FALSE;
		if (instanceVersion >= VK_API_VERSION_1_2) features12.pNext = &features11;
		else addressEnable.pNext = &features11;
	}

	void* featureChain = promoted;
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateEnable = {};
	dynamicStateEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
	dynamicStateEnable.extendedDynamicState = VK_TRUE;

	VkPhysicalDeviceVulkan13Features features13 = {};
	VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering = {};
	VkPhysicalDeviceSynchronization2Features sync2 = {};
	if (coreDynamicRendering)
	{
		features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		features13.pNext = promoted;
		features13.dynamicRendering = VK_TRUE;
		features13.synchronization2 = m_synchronization2 ? VK_TRUE : VK_FALSE;
		features13.shaderDemoteToHelperInvocation = m_shaderDemoteToHelperInvocation ? VK_TRUE : VK_FALSE;
		features13.subgroupSizeControl = m_subgroupSizeControl ? VK_TRUE : VK_FALSE;
		features13.computeFullSubgroups = m_computeFullSubgroups ? VK_TRUE : VK_FALSE;
		features13.shaderIntegerDotProduct = m_shaderIntegerDotProduct ? VK_TRUE : VK_FALSE;
		if (enableExtendedDynamicState)
		{
			dynamicStateEnable.pNext = &features13;
			featureChain = &dynamicStateEnable;
		}
		else featureChain = &features13;
	}
	else
	{
		dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		dynamicRendering.pNext = promoted;
		dynamicRendering.dynamicRendering = VK_TRUE;
		if (m_synchronization2)
		{
			sync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
			sync2.pNext = &dynamicRendering;
			sync2.synchronization2 = VK_TRUE;
			dynamicStateEnable.pNext = &sync2;
		}
		else dynamicStateEnable.pNext = &dynamicRendering;
		featureChain = &dynamicStateEnable;
	}

	VkPhysicalDeviceMeshShaderFeaturesEXT mesh = {};
	if (m_meshShader)
	{
		mesh.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
		mesh.pNext = featureChain;
		mesh.meshShader = VK_TRUE;
		mesh.taskShader = meshTask ? VK_TRUE : VK_FALSE;
		featureChain = &mesh;
	}

	VkPhysicalDeviceFragmentShadingRateFeaturesKHR shadingRate = {};
	if (m_fragmentShadingRate)
	{
		shadingRate.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
		shadingRate.pNext = featureChain;
		shadingRate.pipelineFragmentShadingRate = VK_TRUE;
		shadingRate.primitiveFragmentShadingRate = fsrPrimitive ? VK_TRUE : VK_FALSE;
		shadingRate.attachmentFragmentShadingRate = fsrAttachment ? VK_TRUE : VK_FALSE;
		featureChain = &shadingRate;
	}

	VkPhysicalDeviceCustomBorderColorFeaturesEXT border = {};
	if (m_customBorderColor)
	{
		border.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
		border.pNext = featureChain;
		border.customBorderColors = VK_TRUE;
		border.customBorderColorWithoutFormat = VK_TRUE;
		featureChain = &border;
	}

	VkPhysicalDeviceRobustness2FeaturesKHR robustEnable = {};
	if (m_nullDescriptor)
	{
		robustEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR;
		robustEnable.pNext = featureChain;
		robustEnable.nullDescriptor = VK_TRUE;
		featureChain = &robustEnable;
	}

	VkPhysicalDeviceVertexAttributeDivisorFeatures divisorEnable = {};
	if (m_vertexAttributeDivisor)
	{
		divisorEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES;
		divisorEnable.pNext = featureChain;
		divisorEnable.vertexAttributeInstanceRateDivisor = VK_TRUE;
		featureChain = &divisorEnable;
	}

	VkPhysicalDeviceConditionalRenderingFeaturesEXT conditionalEnable = {};
	if (m_conditionalRendering)
	{
		conditionalEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
		conditionalEnable.pNext = featureChain;
		conditionalEnable.conditionalRendering = VK_TRUE;
		featureChain = &conditionalEnable;
	}

	VkPhysicalDeviceMaintenance5Features maintenance5Enable = {};
	if (m_maintenance5)
	{
		maintenance5Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES;
		maintenance5Enable.pNext = featureChain;
		maintenance5Enable.maintenance5 = VK_TRUE;
		featureChain = &maintenance5Enable;
	}

	VkPhysicalDeviceTransformFeedbackFeaturesEXT xfbEnable = {};
	if (m_transformFeedback)
	{
		xfbEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT;
		xfbEnable.pNext = featureChain;
		xfbEnable.transformFeedback = VK_TRUE;
		xfbEnable.geometryStreams = m_geometryStreams ? VK_TRUE : VK_FALSE;
		featureChain = &xfbEnable;
	}

	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamicState3Enable = {};
	if (m_sampleLocations)
	{
		dynamicState3Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
		dynamicState3Enable.pNext = featureChain;
		dynamicState3Enable.extendedDynamicState3SampleLocationsEnable = VK_TRUE;
		featureChain = &dynamicState3Enable;
	}

	VkPhysicalDevice16BitStorageFeatures storage16Enable = {};
	if (instanceVersion < VK_API_VERSION_1_1 && storage16On)
	{
		storage16Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
		storage16Enable.pNext = featureChain;
		storage16Enable.storageBuffer16BitAccess = m_storageBuffer16BitAccess ? VK_TRUE : VK_FALSE;
		storage16Enable.uniformAndStorageBuffer16BitAccess = m_uniformAndStorageBuffer16BitAccess ? VK_TRUE : VK_FALSE;
		storage16Enable.storagePushConstant16 = m_storagePushConstant16 ? VK_TRUE : VK_FALSE;
		storage16Enable.storageInputOutput16 = m_storageInputOutput16 ? VK_TRUE : VK_FALSE;
		featureChain = &storage16Enable;
	}

	VkPhysicalDeviceShaderDrawParametersFeatures drawParamEnable = {};
	if (instanceVersion < VK_API_VERSION_1_1 && m_shaderDrawParameters)
	{
		drawParamEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
		drawParamEnable.pNext = featureChain;
		drawParamEnable.shaderDrawParameters = VK_TRUE;
		featureChain = &drawParamEnable;
	}

	VkPhysicalDevice8BitStorageFeatures storage8Enable = {};
	if (instanceVersion < VK_API_VERSION_1_2 && storage8On)
	{
		storage8Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
		storage8Enable.pNext = featureChain;
		storage8Enable.storageBuffer8BitAccess = m_storageBuffer8BitAccess ? VK_TRUE : VK_FALSE;
		storage8Enable.uniformAndStorageBuffer8BitAccess = m_uniformAndStorageBuffer8BitAccess ? VK_TRUE : VK_FALSE;
		storage8Enable.storagePushConstant8 = m_storagePushConstant8 ? VK_TRUE : VK_FALSE;
		featureChain = &storage8Enable;
	}

	VkPhysicalDeviceShaderAtomicInt64Features atomicInt64Enable = {};
	if (instanceVersion < VK_API_VERSION_1_2 && (m_shaderBufferInt64Atomics || m_shaderSharedInt64Atomics))
	{
		atomicInt64Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
		atomicInt64Enable.pNext = featureChain;
		atomicInt64Enable.shaderBufferInt64Atomics = m_shaderBufferInt64Atomics ? VK_TRUE : VK_FALSE;
		atomicInt64Enable.shaderSharedInt64Atomics = m_shaderSharedInt64Atomics ? VK_TRUE : VK_FALSE;
		featureChain = &atomicInt64Enable;
	}

	VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures subgroupExtTypesEnable = {};
	if (instanceVersion < VK_API_VERSION_1_2 && m_shaderSubgroupExtendedTypes)
	{
		subgroupExtTypesEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES;
		subgroupExtTypesEnable.pNext = featureChain;
		subgroupExtTypesEnable.shaderSubgroupExtendedTypes = VK_TRUE;
		featureChain = &subgroupExtTypesEnable;
	}

	VkPhysicalDeviceSubgroupSizeControlFeatures subgroupSizeEnable = {};
	if (!coreDynamicRendering && (m_subgroupSizeControl || m_computeFullSubgroups))
	{
		subgroupSizeEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES;
		subgroupSizeEnable.pNext = featureChain;
		subgroupSizeEnable.subgroupSizeControl = m_subgroupSizeControl ? VK_TRUE : VK_FALSE;
		subgroupSizeEnable.computeFullSubgroups = m_computeFullSubgroups ? VK_TRUE : VK_FALSE;
		featureChain = &subgroupSizeEnable;
	}

	VkPhysicalDeviceShaderIntegerDotProductFeatures integerDotEnable = {};
	if (!coreDynamicRendering && m_shaderIntegerDotProduct)
	{
		integerDotEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES;
		integerDotEnable.pNext = featureChain;
		integerDotEnable.shaderIntegerDotProduct = VK_TRUE;
		featureChain = &integerDotEnable;
	}

	VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures demoteEnable = {};
	if (!coreDynamicRendering && m_shaderDemoteToHelperInvocation)
	{
		demoteEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES;
		demoteEnable.pNext = featureChain;
		demoteEnable.shaderDemoteToHelperInvocation = VK_TRUE;
		featureChain = &demoteEnable;
	}

	VkPhysicalDeviceShaderSubgroupRotateFeatures subgroupRotateEnable = {};
	if (m_shaderSubgroupRotate || m_shaderSubgroupRotateClustered)
	{
		subgroupRotateEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_ROTATE_FEATURES;
		subgroupRotateEnable.pNext = featureChain;
		subgroupRotateEnable.shaderSubgroupRotate = m_shaderSubgroupRotate ? VK_TRUE : VK_FALSE;
		subgroupRotateEnable.shaderSubgroupRotateClustered = m_shaderSubgroupRotateClustered ? VK_TRUE : VK_FALSE;
		featureChain = &subgroupRotateEnable;
	}

	VkPhysicalDeviceShaderFloatControls2Features floatControls2Enable = {};
	if (m_shaderFloatControls2)
	{
		floatControls2Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT_CONTROLS_2_FEATURES;
		floatControls2Enable.pNext = featureChain;
		floatControls2Enable.shaderFloatControls2 = VK_TRUE;
		featureChain = &floatControls2Enable;
	}

	VkPhysicalDeviceShaderSubgroupPartitionedFeaturesEXT subgroupPartitionedEnable = {};
	if (m_shaderSubgroupPartitioned)
	{
		subgroupPartitionedEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_PARTITIONED_FEATURES_EXT;
		subgroupPartitionedEnable.pNext = featureChain;
		subgroupPartitionedEnable.shaderSubgroupPartitioned = VK_TRUE;
		featureChain = &subgroupPartitionedEnable;
	}

	VkPhysicalDeviceShaderQuadControlFeaturesKHR quadControlEnable = {};
	if (m_shaderQuadControl)
	{
		quadControlEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_QUAD_CONTROL_FEATURES_KHR;
		quadControlEnable.pNext = featureChain;
		quadControlEnable.shaderQuadControl = VK_TRUE;
		featureChain = &quadControlEnable;
	}

	VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR uniformControlFlowEnable = {};
	if (m_shaderSubgroupUniformControlFlow)
	{
		uniformControlFlowEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR;
		uniformControlFlowEnable.pNext = featureChain;
		uniformControlFlowEnable.shaderSubgroupUniformControlFlow = VK_TRUE;
		featureChain = &uniformControlFlowEnable;
	}

	VkPhysicalDeviceShaderMaximalReconvergenceFeaturesKHR maximalReconvergenceEnable = {};
	if (m_shaderMaximalReconvergence)
	{
		maximalReconvergenceEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MAXIMAL_RECONVERGENCE_FEATURES_KHR;
		maximalReconvergenceEnable.pNext = featureChain;
		maximalReconvergenceEnable.shaderMaximalReconvergence = VK_TRUE;
		featureChain = &maximalReconvergenceEnable;
	}

	VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR computeDerivativesEnable = {};
	if (m_computeDerivativeGroupQuads || m_computeDerivativeGroupLinear)
	{
		computeDerivativesEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_KHR;
		computeDerivativesEnable.pNext = featureChain;
		computeDerivativesEnable.computeDerivativeGroupQuads = m_computeDerivativeGroupQuads ? VK_TRUE : VK_FALSE;
		computeDerivativesEnable.computeDerivativeGroupLinear = m_computeDerivativeGroupLinear ? VK_TRUE : VK_FALSE;
		featureChain = &computeDerivativesEnable;
	}

	VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR barycentricEnable = {};
	if (m_fragmentShaderBarycentric)
	{
		barycentricEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;
		barycentricEnable.pNext = featureChain;
		barycentricEnable.fragmentShaderBarycentric = VK_TRUE;
		featureChain = &barycentricEnable;
	}

	VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT imageAtomicInt64Enable = {};
	if (m_shaderImageInt64Atomics || m_sparseImageInt64Atomics)
	{
		imageAtomicInt64Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT;
		imageAtomicInt64Enable.pNext = featureChain;
		imageAtomicInt64Enable.shaderImageInt64Atomics = m_shaderImageInt64Atomics ? VK_TRUE : VK_FALSE;
		imageAtomicInt64Enable.sparseImageInt64Atomics = m_sparseImageInt64Atomics ? VK_TRUE : VK_FALSE;
		featureChain = &imageAtomicInt64Enable;
	}

	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingEnable = {};
	if (instanceVersion < VK_API_VERSION_1_2 && descriptorIndexingNeeded)
	{
		descriptorIndexingEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		descriptorIndexingEnable.pNext = featureChain;
		descriptorIndexingEnable.shaderInputAttachmentArrayDynamicIndexing = m_shaderInputAttachmentArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
		descriptorIndexingEnable.shaderUniformTexelBufferArrayDynamicIndexing = m_shaderUniformTexelBufferArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
		descriptorIndexingEnable.shaderStorageTexelBufferArrayDynamicIndexing = m_shaderStorageTexelBufferArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
		descriptorIndexingEnable.shaderUniformBufferArrayNonUniformIndexing = m_shaderUniformBufferArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		descriptorIndexingEnable.shaderSampledImageArrayNonUniformIndexing = m_shaderSampledImageArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		descriptorIndexingEnable.shaderStorageBufferArrayNonUniformIndexing = m_shaderStorageBufferArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		descriptorIndexingEnable.shaderStorageImageArrayNonUniformIndexing = m_shaderStorageImageArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		descriptorIndexingEnable.shaderInputAttachmentArrayNonUniformIndexing = m_shaderInputAttachmentArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		descriptorIndexingEnable.shaderUniformTexelBufferArrayNonUniformIndexing = m_shaderUniformTexelBufferArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		descriptorIndexingEnable.shaderStorageTexelBufferArrayNonUniformIndexing = m_shaderStorageTexelBufferArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		descriptorIndexingEnable.runtimeDescriptorArray = m_runtimeDescriptorArray ? VK_TRUE : VK_FALSE;
		descriptorIndexingEnable.descriptorBindingPartiallyBound = m_descriptorBindingPartiallyBound ? VK_TRUE : VK_FALSE;
		featureChain = &descriptorIndexingEnable;
	}

	VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutEnable = {};
	if (instanceVersion < VK_API_VERSION_1_2 && m_scalarBlockLayout)
	{
		scalarBlockLayoutEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
		scalarBlockLayoutEnable.pNext = featureChain;
		scalarBlockLayoutEnable.scalarBlockLayout = VK_TRUE;
		featureChain = &scalarBlockLayoutEnable;
	}

	VkPhysicalDeviceShaderAtomicFloatFeaturesEXT shaderAtomicFloatEnable = {};
	if (shaderAtomicFloatOn || shaderAtomicFloat2On)
	{
		shaderAtomicFloatEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
		shaderAtomicFloatEnable.pNext = featureChain;
		shaderAtomicFloatEnable.shaderBufferFloat32Atomics = m_shaderBufferFloat32Atomics ? VK_TRUE : VK_FALSE;
		shaderAtomicFloatEnable.shaderBufferFloat32AtomicAdd = m_shaderBufferFloat32AtomicAdd ? VK_TRUE : VK_FALSE;
		shaderAtomicFloatEnable.shaderBufferFloat64Atomics = m_shaderBufferFloat64Atomics ? VK_TRUE : VK_FALSE;
		shaderAtomicFloatEnable.shaderBufferFloat64AtomicAdd = m_shaderBufferFloat64AtomicAdd ? VK_TRUE : VK_FALSE;
		shaderAtomicFloatEnable.shaderSharedFloat32Atomics = m_shaderSharedFloat32Atomics ? VK_TRUE : VK_FALSE;
		shaderAtomicFloatEnable.shaderSharedFloat32AtomicAdd = m_shaderSharedFloat32AtomicAdd ? VK_TRUE : VK_FALSE;
		shaderAtomicFloatEnable.shaderSharedFloat64Atomics = m_shaderSharedFloat64Atomics ? VK_TRUE : VK_FALSE;
		shaderAtomicFloatEnable.shaderSharedFloat64AtomicAdd = m_shaderSharedFloat64AtomicAdd ? VK_TRUE : VK_FALSE;
		shaderAtomicFloatEnable.shaderImageFloat32Atomics = m_shaderImageFloat32Atomics ? VK_TRUE : VK_FALSE;
		shaderAtomicFloatEnable.shaderImageFloat32AtomicAdd = m_shaderImageFloat32AtomicAdd ? VK_TRUE : VK_FALSE;
		shaderAtomicFloatEnable.sparseImageFloat32Atomics = m_sparseImageFloat32Atomics ? VK_TRUE : VK_FALSE;
		shaderAtomicFloatEnable.sparseImageFloat32AtomicAdd = m_sparseImageFloat32AtomicAdd ? VK_TRUE : VK_FALSE;
		featureChain = &shaderAtomicFloatEnable;
	}

	VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT shaderAtomicFloat2Enable = {};
	if (shaderAtomicFloat2On)
	{
		shaderAtomicFloat2Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT;
		shaderAtomicFloat2Enable.pNext = featureChain;
		shaderAtomicFloat2Enable.shaderBufferFloat16Atomics = m_shaderBufferFloat16Atomics ? VK_TRUE : VK_FALSE;
		shaderAtomicFloat2Enable.shaderBufferFloat16AtomicAdd = m_shaderBufferFloat16AtomicAdd ? VK_TRUE : VK_FALSE;
		shaderAtomicFloat2Enable.shaderBufferFloat16AtomicMinMax = m_shaderBufferFloat16AtomicMinMax ? VK_TRUE : VK_FALSE;
		shaderAtomicFloat2Enable.shaderBufferFloat32AtomicMinMax = m_shaderBufferFloat32AtomicMinMax ? VK_TRUE : VK_FALSE;
		shaderAtomicFloat2Enable.shaderBufferFloat64AtomicMinMax = m_shaderBufferFloat64AtomicMinMax ? VK_TRUE : VK_FALSE;
		shaderAtomicFloat2Enable.shaderSharedFloat16Atomics = m_shaderSharedFloat16Atomics ? VK_TRUE : VK_FALSE;
		shaderAtomicFloat2Enable.shaderSharedFloat16AtomicAdd = m_shaderSharedFloat16AtomicAdd ? VK_TRUE : VK_FALSE;
		shaderAtomicFloat2Enable.shaderSharedFloat16AtomicMinMax = m_shaderSharedFloat16AtomicMinMax ? VK_TRUE : VK_FALSE;
		shaderAtomicFloat2Enable.shaderSharedFloat32AtomicMinMax = m_shaderSharedFloat32AtomicMinMax ? VK_TRUE : VK_FALSE;
		shaderAtomicFloat2Enable.shaderSharedFloat64AtomicMinMax = m_shaderSharedFloat64AtomicMinMax ? VK_TRUE : VK_FALSE;
		shaderAtomicFloat2Enable.shaderImageFloat32AtomicMinMax = m_shaderImageFloat32AtomicMinMax ? VK_TRUE : VK_FALSE;
		shaderAtomicFloat2Enable.sparseImageFloat32AtomicMinMax = m_sparseImageFloat32AtomicMinMax ? VK_TRUE : VK_FALSE;
		featureChain = &shaderAtomicFloat2Enable;
	}

	VkPhysicalDeviceShaderClockFeaturesKHR shaderClockEnable = {};
	if (m_shaderSubgroupClock || m_shaderDeviceClock)
	{
		shaderClockEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR;
		shaderClockEnable.pNext = featureChain;
		shaderClockEnable.shaderSubgroupClock = m_shaderSubgroupClock ? VK_TRUE : VK_FALSE;
		shaderClockEnable.shaderDeviceClock = m_shaderDeviceClock ? VK_TRUE : VK_FALSE;
		featureChain = &shaderClockEnable;
	}

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureEnable = {};
	if (m_accelerationStructure)
	{
		accelerationStructureEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		accelerationStructureEnable.pNext = featureChain;
		accelerationStructureEnable.accelerationStructure = VK_TRUE;
		accelerationStructureEnable.accelerationStructureCaptureReplay =
			m_accelerationStructureCaptureReplay ? VK_TRUE : VK_FALSE;
		accelerationStructureEnable.accelerationStructureIndirectBuild =
			m_accelerationStructureIndirectBuild ? VK_TRUE : VK_FALSE;
		accelerationStructureEnable.accelerationStructureHostCommands =
			m_accelerationStructureHostCommands ? VK_TRUE : VK_FALSE;
		featureChain = &accelerationStructureEnable;
	}

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineEnable = {};
	if (m_rayTracingPipeline)
	{
		rayTracingPipelineEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		rayTracingPipelineEnable.pNext = featureChain;
		rayTracingPipelineEnable.rayTracingPipeline = VK_TRUE;
		rayTracingPipelineEnable.rayTracingPipelineShaderGroupHandleCaptureReplay =
			m_rayTracingPipelineShaderGroupHandleCaptureReplay ? VK_TRUE : VK_FALSE;
		rayTracingPipelineEnable.rayTracingPipelineShaderGroupHandleCaptureReplayMixed =
			m_rayTracingPipelineShaderGroupHandleCaptureReplayMixed ? VK_TRUE : VK_FALSE;
		rayTracingPipelineEnable.rayTracingPipelineTraceRaysIndirect =
			m_rayTracingPipelineTraceRaysIndirect ? VK_TRUE : VK_FALSE;
		rayTracingPipelineEnable.rayTraversalPrimitiveCulling = m_rayTraversalPrimitiveCulling ? VK_TRUE : VK_FALSE;
		featureChain = &rayTracingPipelineEnable;
	}

	VkPhysicalDeviceRayQueryFeaturesKHR rayQueryEnable = {};
	if (m_rayQuery)
	{
		rayQueryEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
		rayQueryEnable.pNext = featureChain;
		rayQueryEnable.rayQuery = VK_TRUE;
		featureChain = &rayQueryEnable;
	}

	VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR rayTracingMaintenance1Enable = {};
	if (m_rayTracingMaintenance1 || m_rayTracingPipelineTraceRaysIndirect2)
	{
		rayTracingMaintenance1Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR;
		rayTracingMaintenance1Enable.pNext = featureChain;
		rayTracingMaintenance1Enable.rayTracingMaintenance1 = m_rayTracingMaintenance1 ? VK_TRUE : VK_FALSE;
		rayTracingMaintenance1Enable.rayTracingPipelineTraceRaysIndirect2 =
			m_rayTracingPipelineTraceRaysIndirect2 ? VK_TRUE : VK_FALSE;
		featureChain = &rayTracingMaintenance1Enable;
	}

	VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR rayTracingPositionFetchEnable = {};
	if (m_rayTracingPositionFetch)
	{
		rayTracingPositionFetchEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR;
		rayTracingPositionFetchEnable.pNext = featureChain;
		rayTracingPositionFetchEnable.rayTracingPositionFetch = VK_TRUE;
		featureChain = &rayTracingPositionFetchEnable;
	}

	VkPhysicalDeviceOpacityMicromapFeaturesKHR opacityMicromapEnable = {};
	VkPhysicalDeviceOpacityMicromapFeaturesEXT opacityMicromapExtEnable = {};
	VkPhysicalDeviceIndexTypeUint8Features indexTypeUint8Enable = {};
	if (m_indexTypeUint8)
	{
		indexTypeUint8Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES;
		indexTypeUint8Enable.pNext = featureChain;
		indexTypeUint8Enable.indexTypeUint8 = VK_TRUE;
		featureChain = &indexTypeUint8Enable;
	}
	if (m_micromap && opacityMicromapKhr)
	{
		opacityMicromapEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_KHR;
		opacityMicromapEnable.pNext = featureChain;
		opacityMicromapEnable.micromap = VK_TRUE;
		featureChain = &opacityMicromapEnable;
	}
	else if (m_micromap)
	{
		opacityMicromapExtEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_EXT;
		opacityMicromapExtEnable.pNext = featureChain;
		opacityMicromapExtEnable.micromap = VK_TRUE;
		opacityMicromapExtEnable.micromapCaptureReplay = m_micromapCaptureReplay ? VK_TRUE : VK_FALSE;
		opacityMicromapExtEnable.micromapHostCommands = m_micromapHostCommands ? VK_TRUE : VK_FALSE;
		featureChain = &opacityMicromapExtEnable;
	}

	VkPhysicalDeviceRayTracingInvocationReorderFeaturesEXT invocationReorderEnable = {};
	if (m_rayTracingInvocationReorder)
	{
		invocationReorderEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_FEATURES_EXT;
		invocationReorderEnable.pNext = featureChain;
		invocationReorderEnable.rayTracingInvocationReorder = VK_TRUE;
		featureChain = &invocationReorderEnable;
	}

	VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesKHR pipelineLibraryGroupHandlesEnable = {};
	if (m_pipelineLibraryGroupHandles)
	{
		pipelineLibraryGroupHandlesEnable.sType =
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_LIBRARY_GROUP_HANDLES_FEATURES_KHR;
		pipelineLibraryGroupHandlesEnable.pNext = featureChain;
		pipelineLibraryGroupHandlesEnable.pipelineLibraryGroupHandles = VK_TRUE;
		featureChain = &pipelineLibraryGroupHandlesEnable;
	}

	VkPhysicalDeviceCooperativeMatrixFeaturesKHR cooperativeMatrixEnable = {};
	if (m_cooperativeMatrix)
	{
		cooperativeMatrixEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR;
		cooperativeMatrixEnable.pNext = featureChain;
		cooperativeMatrixEnable.cooperativeMatrix = VK_TRUE;
		cooperativeMatrixEnable.cooperativeMatrixRobustBufferAccess =
			m_cooperativeMatrixRobustBufferAccess ? VK_TRUE : VK_FALSE;
		featureChain = &cooperativeMatrixEnable;
	}

	VkPhysicalDeviceShaderBfloat16FeaturesKHR shaderBfloat16Enable = {};
	if (m_shaderBFloat16Type || m_shaderBFloat16DotProduct || m_shaderBFloat16CooperativeMatrix)
	{
		shaderBfloat16Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_BFLOAT16_FEATURES_KHR;
		shaderBfloat16Enable.pNext = featureChain;
		shaderBfloat16Enable.shaderBFloat16Type = m_shaderBFloat16Type ? VK_TRUE : VK_FALSE;
		shaderBfloat16Enable.shaderBFloat16DotProduct = m_shaderBFloat16DotProduct ? VK_TRUE : VK_FALSE;
		shaderBfloat16Enable.shaderBFloat16CooperativeMatrix = m_shaderBFloat16CooperativeMatrix ? VK_TRUE : VK_FALSE;
		featureChain = &shaderBfloat16Enable;
	}

	VkPhysicalDeviceShaderFloat8FeaturesEXT shaderFloat8Enable = {};
	if (m_shaderFloat8 || m_shaderFloat8CooperativeMatrix)
	{
		shaderFloat8Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT8_FEATURES_EXT;
		shaderFloat8Enable.pNext = featureChain;
		shaderFloat8Enable.shaderFloat8 = m_shaderFloat8 ? VK_TRUE : VK_FALSE;
		shaderFloat8Enable.shaderFloat8CooperativeMatrix = m_shaderFloat8CooperativeMatrix ? VK_TRUE : VK_FALSE;
		featureChain = &shaderFloat8Enable;
	}

	VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR workgroupLayoutEnable = {};
	if (m_workgroupMemoryExplicitLayout || m_workgroupMemoryExplicitLayoutScalarBlockLayout ||
		m_workgroupMemoryExplicitLayout8BitAccess || m_workgroupMemoryExplicitLayout16BitAccess)
	{
		workgroupLayoutEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR;
		workgroupLayoutEnable.pNext = featureChain;
		workgroupLayoutEnable.workgroupMemoryExplicitLayout = m_workgroupMemoryExplicitLayout ? VK_TRUE : VK_FALSE;
		workgroupLayoutEnable.workgroupMemoryExplicitLayoutScalarBlockLayout =
			m_workgroupMemoryExplicitLayoutScalarBlockLayout ? VK_TRUE : VK_FALSE;
		workgroupLayoutEnable.workgroupMemoryExplicitLayout8BitAccess =
			m_workgroupMemoryExplicitLayout8BitAccess ? VK_TRUE : VK_FALSE;
		workgroupLayoutEnable.workgroupMemoryExplicitLayout16BitAccess =
			m_workgroupMemoryExplicitLayout16BitAccess ? VK_TRUE : VK_FALSE;
		featureChain = &workgroupLayoutEnable;
	}

	VkPhysicalDeviceShaderLongVectorFeaturesEXT longVectorEnable = {};
	if (m_longVector)
	{
		longVectorEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_LONG_VECTOR_FEATURES_EXT;
		longVectorEnable.pNext = featureChain;
		longVectorEnable.longVector = VK_TRUE;
		featureChain = &longVectorEnable;
	}

	VkPhysicalDeviceCooperativeMatrixMaintenance1FeaturesEXT cooperativeMatrixMaintenance1Enable = {};
	if (m_cooperativeMatrixProperties2 || m_cooperativeMatrixReductions || m_cooperativeMatrixConversions ||
		m_cooperativeMatrixPerElementOperations || m_cooperativeMatrixGetCoordinate)
	{
		cooperativeMatrixMaintenance1Enable.sType =
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_MAINTENANCE_1_FEATURES_EXT;
		cooperativeMatrixMaintenance1Enable.pNext = featureChain;
		cooperativeMatrixMaintenance1Enable.cooperativeMatrixProperties2 =
			m_cooperativeMatrixProperties2 ? VK_TRUE : VK_FALSE;
		cooperativeMatrixMaintenance1Enable.cooperativeMatrixReductions =
			m_cooperativeMatrixReductions ? VK_TRUE : VK_FALSE;
		cooperativeMatrixMaintenance1Enable.cooperativeMatrixConversions =
			m_cooperativeMatrixConversions ? VK_TRUE : VK_FALSE;
		cooperativeMatrixMaintenance1Enable.cooperativeMatrixPerElementOperations =
			m_cooperativeMatrixPerElementOperations ? VK_TRUE : VK_FALSE;
		cooperativeMatrixMaintenance1Enable.cooperativeMatrixGetCoordinate =
			m_cooperativeMatrixGetCoordinate ? VK_TRUE : VK_FALSE;
		featureChain = &cooperativeMatrixMaintenance1Enable;
	}

	const float priorities[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	VkDeviceQueueCreateInfo queueInfos[3] = {};
	for (auto i = 0u; i < m_poolCount; ++i)
	{
		queueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfos[i].queueFamilyIndex = m_queuePools[i].Family;
		queueInfos[i].queueCount = m_queuePools[i].Count;
		queueInfos[i].pQueuePriorities = priorities;
	}

	VkPhysicalDeviceFeatures supported = {};
	vkGetPhysicalDeviceFeatures(m_physicalDevice, &supported);
	if (!supported.samplerAnisotropy || !supported.independentBlend ||
		!supported.fillModeNonSolid || !supported.shaderImageGatherExtended)
		return false;

	VkPhysicalDeviceFeatures enabledFeatures = {};
	enabledFeatures.samplerAnisotropy = supported.samplerAnisotropy;
	enabledFeatures.shaderImageGatherExtended = supported.shaderImageGatherExtended;
	enabledFeatures.geometryShader = supported.geometryShader;
	enabledFeatures.tessellationShader = supported.tessellationShader;
	enabledFeatures.fillModeNonSolid = supported.fillModeNonSolid;
	enabledFeatures.independentBlend = supported.independentBlend;
	enabledFeatures.multiDrawIndirect = supported.multiDrawIndirect;
	enabledFeatures.depthClamp = supported.depthClamp;
	enabledFeatures.fragmentStoresAndAtomics = supported.fragmentStoresAndAtomics;
	enabledFeatures.robustBufferAccess = m_cooperativeMatrixRobustBufferAccess ? VK_TRUE : VK_FALSE;
	enabledFeatures.shaderInt64 = supported.shaderInt64;
	enabledFeatures.shaderInt16 = m_shaderInt16 ? VK_TRUE : VK_FALSE;
	enabledFeatures.shaderFloat64 = m_shaderFloat64 ? VK_TRUE : VK_FALSE;
	enabledFeatures.shaderStorageImageReadWithoutFormat = m_shaderStorageImageReadWithoutFormat ? VK_TRUE : VK_FALSE;
	enabledFeatures.shaderStorageImageExtendedFormats = m_shaderStorageImageExtendedFormats ? VK_TRUE : VK_FALSE;
	enabledFeatures.shaderStorageImageMultisample = m_shaderStorageImageMultisample ? VK_TRUE : VK_FALSE;
	enabledFeatures.shaderClipDistance = m_shaderClipDistance ? VK_TRUE : VK_FALSE;
	enabledFeatures.shaderCullDistance = m_shaderCullDistance ? VK_TRUE : VK_FALSE;
	enabledFeatures.vertexPipelineStoresAndAtomics = m_vertexPipelineStoresAndAtomics ? VK_TRUE : VK_FALSE;
	enabledFeatures.sampleRateShading = m_sampleRateShading ? VK_TRUE : VK_FALSE;
	enabledFeatures.imageCubeArray = m_imageCubeArray ? VK_TRUE : VK_FALSE;
	enabledFeatures.textureCompressionBC = m_textureCompressionBC ? VK_TRUE : VK_FALSE;
	enabledFeatures.drawIndirectFirstInstance = m_drawIndirectFirstInstance ? VK_TRUE : VK_FALSE;
	enabledFeatures.shaderUniformBufferArrayDynamicIndexing = m_shaderUniformBufferArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
	enabledFeatures.shaderSampledImageArrayDynamicIndexing = m_shaderSampledImageArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
	enabledFeatures.shaderStorageBufferArrayDynamicIndexing = m_shaderStorageBufferArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
	enabledFeatures.shaderStorageImageArrayDynamicIndexing = m_shaderStorageImageArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
	enabledFeatures.shaderStorageImageWriteWithoutFormat = supported.shaderStorageImageWriteWithoutFormat;
	enabledFeatures.sparseBinding = supported.sparseBinding;
	enabledFeatures.sparseResidencyBuffer = supported.sparseResidencyBuffer;
	enabledFeatures.sparseResidencyImage2D = supported.sparseResidencyImage2D;
	enabledFeatures.sparseResidencyImage3D = supported.sparseResidencyImage3D;
	enabledFeatures.sparseResidency2Samples = supported.sparseResidency2Samples;
	enabledFeatures.sparseResidency4Samples = supported.sparseResidency4Samples;
	enabledFeatures.sparseResidency8Samples = supported.sparseResidency8Samples;
	enabledFeatures.sparseResidency16Samples = supported.sparseResidency16Samples;
	enabledFeatures.sparseResidencyAliased = supported.sparseResidencyAliased;
	m_sparseBinding = supported.sparseBinding == VK_TRUE;
	m_sparseResidencyBuffer = supported.sparseResidencyBuffer == VK_TRUE;
	m_sparseResidencyImage2D = supported.sparseResidencyImage2D == VK_TRUE;
	m_sparseResidencyImage3D = supported.sparseResidencyImage3D == VK_TRUE;
	m_sparseResidency2Samples = supported.sparseResidency2Samples == VK_TRUE;
	m_sparseResidency4Samples = supported.sparseResidency4Samples == VK_TRUE;
	m_sparseResidency8Samples = supported.sparseResidency8Samples == VK_TRUE;
	m_sparseResidency16Samples = supported.sparseResidency16Samples == VK_TRUE;
	m_sparseResidencyAliased = supported.sparseResidencyAliased == VK_TRUE;

	VkDeviceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	info.pNext = featureChain;
	info.queueCreateInfoCount = m_poolCount;
	info.pQueueCreateInfos = queueInfos;
	info.enabledExtensionCount = static_cast<uint32_t>(enabled.size());
	info.ppEnabledExtensionNames = enabled.data();
	info.pEnabledFeatures = &enabledFeatures;

	const auto deviceResult = vkCreateDevice(m_physicalDevice, &info, nullptr, &m_device);
	if (deviceResult < 0)
	{
		cerr << "VkResult " << deviceResult << endl;

		return false;
	}

	if (coreDynamicRendering)
	{
		m_beginRendering = vkCmdBeginRendering;
		m_endRendering = vkCmdEndRendering;
		m_setPrimitiveTopology = vkCmdSetPrimitiveTopology;
		m_bindVertexBuffers2 = vkCmdBindVertexBuffers2;
	}
	else
	{
		m_beginRendering = reinterpret_cast<PFN_vkCmdBeginRendering>(vkGetDeviceProcAddr(m_device, "vkCmdBeginRenderingKHR"));
		m_endRendering = reinterpret_cast<PFN_vkCmdEndRendering>(vkGetDeviceProcAddr(m_device, "vkCmdEndRenderingKHR"));
		m_setPrimitiveTopology = reinterpret_cast<PFN_vkCmdSetPrimitiveTopology>(vkGetDeviceProcAddr(m_device, "vkCmdSetPrimitiveTopologyEXT"));
		m_bindVertexBuffers2 = reinterpret_cast<PFN_vkCmdBindVertexBuffers2>(vkGetDeviceProcAddr(m_device, "vkCmdBindVertexBuffers2EXT"));
	}
	assert(m_beginRendering);
	assert(m_endRendering);
	assert(m_setPrimitiveTopology);

	m_ext.SetDebugName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
		vkGetInstanceProcAddr(m_instance, "vkSetDebugUtilsObjectNameEXT"));
	if (m_deviceGeneratedCommands)
	{
		m_ext.CreateIndirectCommandsLayout = reinterpret_cast<PFN_vkCreateIndirectCommandsLayoutEXT>(
			vkGetDeviceProcAddr(m_device, "vkCreateIndirectCommandsLayoutEXT"));
		m_ext.DestroyIndirectCommandsLayout = reinterpret_cast<PFN_vkDestroyIndirectCommandsLayoutEXT>(
			vkGetDeviceProcAddr(m_device, "vkDestroyIndirectCommandsLayoutEXT"));
		m_ext.GetGeneratedCommandsMemoryRequirements = reinterpret_cast<PFN_vkGetGeneratedCommandsMemoryRequirementsEXT>(
			vkGetDeviceProcAddr(m_device, "vkGetGeneratedCommandsMemoryRequirementsEXT"));
		m_ext.PreprocessGeneratedCommands = reinterpret_cast<PFN_vkCmdPreprocessGeneratedCommandsEXT>(
			vkGetDeviceProcAddr(m_device, "vkCmdPreprocessGeneratedCommandsEXT"));
		m_ext.ExecuteGeneratedCommands = reinterpret_cast<PFN_vkCmdExecuteGeneratedCommandsEXT>(
			vkGetDeviceProcAddr(m_device, "vkCmdExecuteGeneratedCommandsEXT"));
		if (!m_ext.CreateIndirectCommandsLayout || !m_ext.DestroyIndirectCommandsLayout ||
			!m_ext.GetGeneratedCommandsMemoryRequirements || !m_ext.PreprocessGeneratedCommands ||
			!m_ext.ExecuteGeneratedCommands)
			m_deviceGeneratedCommands = false;
	}
	m_ext.BindSamplerHeap = reinterpret_cast<PFN_vkCmdBindSamplerHeapEXT>(vkGetDeviceProcAddr(m_device, "vkCmdBindSamplerHeapEXT"));
	m_ext.BindResourceHeap = reinterpret_cast<PFN_vkCmdBindResourceHeapEXT>(vkGetDeviceProcAddr(m_device, "vkCmdBindResourceHeapEXT"));
	m_ext.PushData = reinterpret_cast<PFN_vkCmdPushDataEXT>(vkGetDeviceProcAddr(m_device, "vkCmdPushDataEXT"));
	m_ext.WriteSamplerDescriptors = reinterpret_cast<PFN_vkWriteSamplerDescriptorsEXT>(
		vkGetDeviceProcAddr(m_device, "vkWriteSamplerDescriptorsEXT"));
	m_ext.WriteResourceDescriptors = reinterpret_cast<PFN_vkWriteResourceDescriptorsEXT>(
		vkGetDeviceProcAddr(m_device, "vkWriteResourceDescriptorsEXT"));
	assert(!m_deviceGeneratedCommands || m_ext.CreateIndirectCommandsLayout);
	assert(!m_deviceGeneratedCommands || m_ext.DestroyIndirectCommandsLayout);
	assert(m_ext.BindSamplerHeap);
	assert(m_ext.BindResourceHeap);
	assert(m_ext.PushData);
	assert(!m_deviceGeneratedCommands || m_ext.GetGeneratedCommandsMemoryRequirements);
	assert(!m_deviceGeneratedCommands || m_ext.PreprocessGeneratedCommands);
	assert(!m_deviceGeneratedCommands || m_ext.ExecuteGeneratedCommands);
	m_ext.TraceRaysIndirect2 = nullptr;
	if (m_rayTracingPipelineTraceRaysIndirect2)
	{
		m_ext.TraceRaysIndirect2 = reinterpret_cast<PFN_vkCmdTraceRaysIndirect2KHR>(
			vkGetDeviceProcAddr(m_device, "vkCmdTraceRaysIndirect2KHR"));
		if (!m_ext.TraceRaysIndirect2) m_rayTracingPipelineTraceRaysIndirect2 = false;
	}
	m_ext.CreateAccelerationStructure = nullptr;
	m_ext.DestroyAccelerationStructure = nullptr;
	m_ext.GetAccelerationStructureBuildSizes = nullptr;
	m_ext.GetAccelerationStructureDeviceAddress = nullptr;
	m_ext.CmdBuildAccelerationStructures = nullptr;
	m_ext.CmdCopyAccelerationStructure = nullptr;
	m_ext.CmdWriteAccelerationStructuresProperties = nullptr;
	m_ext.CreateRayTracingPipelines = nullptr;
	m_ext.GetRayTracingShaderGroupHandles = nullptr;
	m_ext.CmdTraceRays = nullptr;
	if (m_accelerationStructure)
	{
		m_ext.CreateAccelerationStructure = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
			vkGetDeviceProcAddr(m_device, "vkCreateAccelerationStructureKHR"));
		m_ext.DestroyAccelerationStructure = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
			vkGetDeviceProcAddr(m_device, "vkDestroyAccelerationStructureKHR"));
		m_ext.GetAccelerationStructureBuildSizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
			vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureBuildSizesKHR"));
		m_ext.GetAccelerationStructureDeviceAddress = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
			vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureDeviceAddressKHR"));
		m_ext.CmdBuildAccelerationStructures = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
			vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR"));
		m_ext.CmdCopyAccelerationStructure = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureKHR>(
			vkGetDeviceProcAddr(m_device, "vkCmdCopyAccelerationStructureKHR"));
		m_ext.CmdWriteAccelerationStructuresProperties = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(
			vkGetDeviceProcAddr(m_device, "vkCmdWriteAccelerationStructuresPropertiesKHR"));
		if (!m_ext.CreateAccelerationStructure || !m_ext.DestroyAccelerationStructure ||
			!m_ext.GetAccelerationStructureBuildSizes || !m_ext.GetAccelerationStructureDeviceAddress ||
			!m_ext.CmdBuildAccelerationStructures || !m_ext.CmdCopyAccelerationStructure ||
			!m_ext.CmdWriteAccelerationStructuresProperties)
			m_accelerationStructure = false;
	}
	if (!m_accelerationStructure) m_rayTracingPipeline = false;
	if (m_rayTracingPipeline)
	{
		m_ext.CreateRayTracingPipelines = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
			vkGetDeviceProcAddr(m_device, "vkCreateRayTracingPipelinesKHR"));
		m_ext.GetRayTracingShaderGroupHandles = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
			vkGetDeviceProcAddr(m_device, "vkGetRayTracingShaderGroupHandlesKHR"));
		m_ext.CmdTraceRays = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
			vkGetDeviceProcAddr(m_device, "vkCmdTraceRaysKHR"));
		if (!m_ext.CreateRayTracingPipelines || !m_ext.GetRayTracingShaderGroupHandles || !m_ext.CmdTraceRays)
			m_rayTracingPipeline = false;
	}
	if (!m_rayTracingPipeline)
	{
		m_rayTracingPipelineTraceRaysIndirect = false;
		m_rayTracingPipelineTraceRaysIndirect2 = false;
		m_ext.TraceRaysIndirect2 = nullptr;
	}
	assert(m_ext.WriteSamplerDescriptors);
	assert(m_ext.WriteResourceDescriptors);
	if (m_conditionalRendering)
	{
		m_ext.BeginConditionalRendering = reinterpret_cast<PFN_vkCmdBeginConditionalRenderingEXT>(
			vkGetDeviceProcAddr(m_device, "vkCmdBeginConditionalRenderingEXT"));
		m_ext.EndConditionalRendering = reinterpret_cast<PFN_vkCmdEndConditionalRenderingEXT>(
			vkGetDeviceProcAddr(m_device, "vkCmdEndConditionalRenderingEXT"));
		assert(m_ext.BeginConditionalRendering);
		assert(m_ext.EndConditionalRendering);
	}
	if (m_transformFeedback)
	{
		m_ext.BindTransformFeedbackBuffers = reinterpret_cast<PFN_vkCmdBindTransformFeedbackBuffersEXT>(
			vkGetDeviceProcAddr(m_device, "vkCmdBindTransformFeedbackBuffersEXT"));
		m_ext.BeginTransformFeedback = reinterpret_cast<PFN_vkCmdBeginTransformFeedbackEXT>(
			vkGetDeviceProcAddr(m_device, "vkCmdBeginTransformFeedbackEXT"));
		m_ext.EndTransformFeedback = reinterpret_cast<PFN_vkCmdEndTransformFeedbackEXT>(
			vkGetDeviceProcAddr(m_device, "vkCmdEndTransformFeedbackEXT"));
		m_ext.BeginQueryIndexed = reinterpret_cast<PFN_vkCmdBeginQueryIndexedEXT>(
			vkGetDeviceProcAddr(m_device, "vkCmdBeginQueryIndexedEXT"));
		m_ext.EndQueryIndexed = reinterpret_cast<PFN_vkCmdEndQueryIndexedEXT>(
			vkGetDeviceProcAddr(m_device, "vkCmdEndQueryIndexedEXT"));
		assert(m_ext.BindTransformFeedbackBuffers);
		assert(m_ext.BeginTransformFeedback);
		assert(m_ext.EndTransformFeedback);
		assert(m_ext.BeginQueryIndexed);
		assert(m_ext.EndQueryIndexed);
	}
	if (m_sampleLocations)
	{
		m_ext.SetSampleLocations = reinterpret_cast<PFN_vkCmdSetSampleLocationsEXT>(
			vkGetDeviceProcAddr(m_device, "vkCmdSetSampleLocationsEXT"));
		m_ext.SetSampleLocationsEnable = reinterpret_cast<PFN_vkCmdSetSampleLocationsEnableEXT>(
			vkGetDeviceProcAddr(m_device, "vkCmdSetSampleLocationsEnableEXT"));
		m_ext.GetMultisampleProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT>(
			vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceMultisamplePropertiesEXT"));
		assert(m_ext.SetSampleLocations);
		assert(m_ext.SetSampleLocationsEnable);
		assert(m_ext.GetMultisampleProperties);
		if (!m_ext.SetSampleLocations || !m_ext.SetSampleLocationsEnable || !m_ext.GetMultisampleProperties)
			m_sampleLocations = false;
		else
		{
			const VkSampleCountFlagBits counts[] =
			{
				VK_SAMPLE_COUNT_1_BIT,
				VK_SAMPLE_COUNT_2_BIT,
				VK_SAMPLE_COUNT_4_BIT,
				VK_SAMPLE_COUNT_8_BIT,
				VK_SAMPLE_COUNT_16_BIT
			};
			for (auto i = 0u; i < 5u; ++i)
			{
				m_sampleLocationGrids[i] = {};
				if ((m_sampleLocationCounts & counts[i]) == 0) continue;
				VkMultisamplePropertiesEXT multi = {};
				multi.sType = VK_STRUCTURE_TYPE_MULTISAMPLE_PROPERTIES_EXT;
				m_ext.GetMultisampleProperties(m_physicalDevice, counts[i], &multi);
				m_sampleLocationGrids[i] = multi.maxSampleLocationGridSize;
			}
		}
	}

	return true;
}

//--------------------------------------------------------------------------------------

Fence_VK::Fence_VK() :
	m_pDevice(nullptr),
	m_semaphore(VK_NULL_HANDLE)
{
}

Fence_VK::~Fence_VK()
{
	if (m_semaphore != VK_NULL_HANDLE && m_pDevice) vkDestroySemaphore(m_pDevice->GetDevice(), m_semaphore, nullptr);
}

bool Fence_VK::Create(Device* pDevice, uint64_t initialValue, FenceFlag flags, const wchar_t* name)
{
	(void)flags;
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
	const auto device = m_pDevice->GetDevice();

	VkSemaphoreTypeCreateInfo type = {};
	type.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	type.initialValue = initialValue;

	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	info.pNext = &type;

	XUSG_VK_CHECK(vkCreateSemaphore(device, &info, nullptr, &m_semaphore), cerr, false);
	m_pDevice->SetDebugName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)m_semaphore, name);

	return true;
}

bool Fence_VK::SetEventOnCompletion(uint64_t value, void* hEvent)
{
	// Wait on this thread. Spawning a worker just to SetEvent then WaitForSingleObject
	// on the caller adds thread create/join cost every frame.
	VkSemaphoreWaitInfo wait = {};
	wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	wait.semaphoreCount = 1;
	wait.pSemaphores = &m_semaphore;
	wait.pValues = &value;
	XUSG_VK_CHECK(vkWaitSemaphores(m_pDevice->GetDevice(), &wait, UINT64_MAX), cerr, false);
	if (hEvent) XUSG_SIGNAL_EVENT(hEvent);

	return true;
}

bool Fence_VK::Signal(uint64_t value)
{
	VkSemaphoreSignalInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
	info.semaphore = m_semaphore;
	info.value = value;
	XUSG_VK_CHECK(vkSignalSemaphore(m_pDevice->GetDevice(), &info), cerr, false);

	return true;
}

uint64_t Fence_VK::GetCompletedValue() const
{
	uint64_t value = 0;
	vkGetSemaphoreCounterValue(m_pDevice->GetDevice(), m_semaphore, &value);

	return value;
}

void Fence_VK::Create(uint64_t nativeHandle, Device* pDevice)
{
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);

	m_semaphore = (VkSemaphore)nativeHandle;
}

uint64_t Fence_VK::GetNativeHandle() const
{
	return (uint64_t)m_semaphore;
}

VkSemaphore Fence_VK::GetSemaphore() const
{
	return m_semaphore;
}

//--------------------------------------------------------------------------------------

CommandLayout_VK::CommandLayout_VK() :
	m_pDevice(nullptr),
	m_layout(VK_NULL_HANDLE),
	m_stages(VK_SHADER_STAGE_ALL),
	m_stride(0),
	m_isDispatch(false),
	m_traceRays(false),
	m_traceRaysOnly(false)
{
}

CommandLayout_VK::~CommandLayout_VK()
{
	if (m_layout != VK_NULL_HANDLE && m_pDevice) m_pDevice->GetExtProcs().DestroyIndirectCommandsLayout(m_pDevice->GetDevice(), m_layout, nullptr);
}

bool CommandLayout_VK::Create(Device* pDevice, uint32_t byteStride, uint32_t numArguments,
	const IndirectArgument* pArguments, const PipelineLayout& pipelineLayout, uint32_t nodeMask,
	const wchar_t* name)
{
	(void)nodeMask;
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
	const auto traceRaysIndirect2 = m_pDevice->HasRayTracingPipelineTraceRaysIndirect2();
	const auto deviceGeneratedCommands = m_pDevice->HasDeviceGeneratedCommands();

	const auto pLayout = static_cast<const PipelineLayoutObject*>(pipelineLayout);
	m_tokens.resize(numArguments);
	m_vertexTokens.clear();
	m_indexTokens.clear();
	m_pushTokens.clear();

	auto offset = 0u;
	m_isDispatch = false;
	m_traceRays = false;
	m_traceRaysOnly = false;
	auto graphics = false;
	for (auto i = 0u; i < numArguments; ++i)
	{
		auto& token = m_tokens[i];
		token = {};
		token.sType = VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_TOKEN_EXT;
		token.offset = offset;

		switch (pArguments[i].Type)
		{
		case IndirectArgumentType::DRAW:
			token.type = VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_EXT;
			offset += sizeof(VkDrawIndirectCommand);
			graphics = true;
			break;
		case IndirectArgumentType::DRAW_INDEXED:
			token.type = VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_EXT;
			offset += sizeof(VkDrawIndexedIndirectCommand);
			graphics = true;
			break;
		case IndirectArgumentType::DISPATCH:
			token.type = VK_INDIRECT_COMMANDS_TOKEN_TYPE_DISPATCH_EXT;
			offset += sizeof(VkDispatchIndirectCommand);
			m_isDispatch = true;
			break;
		case IndirectArgumentType::DISPATCH_RAYS:
			assert(traceRaysIndirect2);
			if (!traceRaysIndirect2) return false;
			assert(!m_traceRays && !graphics && !m_isDispatch);
			if (m_traceRays || graphics || m_isDispatch) return false;
			static_assert(sizeof(VkTraceRaysIndirectCommand2KHR) == sizeof(D3D12_DISPATCH_RAYS_DESC));
			static_assert(offsetof(VkTraceRaysIndirectCommand2KHR, depth) == offsetof(D3D12_DISPATCH_RAYS_DESC, Depth));
			token.type = VK_INDIRECT_COMMANDS_TOKEN_TYPE_TRACE_RAYS2_EXT;
			offset += sizeof(VkTraceRaysIndirectCommand2KHR);
			m_traceRays = true;
			break;
		case IndirectArgumentType::DISPATCH_MESH:
			token.type = VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_MESH_TASKS_EXT;
			offset += sizeof(VkDispatchIndirectCommand);
			graphics = true;
			break;
		case IndirectArgumentType::VERTEX_BUFFER_VIEW:
		{
			VkIndirectCommandsVertexBufferTokenEXT vertex = {};
			vertex.vertexBindingUnit = pArguments[i].VertexBuffer.Slot;
			m_vertexTokens.push_back(vertex);
			token.type = VK_INDIRECT_COMMANDS_TOKEN_TYPE_VERTEX_BUFFER_EXT;
			token.data.pVertexBuffer = &m_vertexTokens.back();
			offset += sizeof(VkBindVertexBufferIndirectCommandEXT);
			graphics = true;
			break;
		}
		case IndirectArgumentType::INDEX_BUFFER_VIEW:
		{
			VkIndirectCommandsIndexBufferTokenEXT index = {};
			index.mode = VK_INDIRECT_COMMANDS_INPUT_MODE_DXGI_INDEX_BUFFER_EXT;
			m_indexTokens.push_back(index);
			token.type = VK_INDIRECT_COMMANDS_TOKEN_TYPE_INDEX_BUFFER_EXT;
			token.data.pIndexBuffer = &m_indexTokens.back();
			offset += sizeof(VkBindIndexBufferIndirectCommandEXT);
			graphics = true;
			break;
		}
		case IndirectArgumentType::CONSTANT:
		case IndirectArgumentType::INCREMENTING_CONSTANT:
		{
			assert(pLayout);
			const auto& root = pLayout->RootParameters[pArguments[i].Constant.Index];
			VkIndirectCommandsPushConstantTokenEXT push = {};
			push.updateRange.stageFlags = VK_SHADER_STAGE_ALL;
			push.updateRange.offset = root.PushByteOffset + pArguments[i].Constant.DestOffsetIn32BitValues * sizeof(uint32_t);
			push.updateRange.size = pArguments[i].Type == IndirectArgumentType::INCREMENTING_CONSTANT ?
				sizeof(uint32_t) : pArguments[i].Constant.Num32BitValuesToSet * sizeof(uint32_t);
			m_pushTokens.push_back(push);
			token.type = pArguments[i].Type == IndirectArgumentType::INCREMENTING_CONSTANT ?
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_DATA_SEQUENCE_INDEX_EXT :
				VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_DATA_EXT;
			token.data.pPushConstant = &m_pushTokens.back();
			if (pArguments[i].Type == IndirectArgumentType::CONSTANT)
				offset += push.updateRange.size;
			break;
		}
		case IndirectArgumentType::CONSTANT_BUFFER_VIEW:
		case IndirectArgumentType::SHADER_RESOURCE_VIEW:
		case IndirectArgumentType::UNORDERED_ACCESS_VIEW:
		{
			assert(pLayout);
			const auto& root = pLayout->RootParameters[pArguments[i].CbvSrvUav.Index];
			VkIndirectCommandsPushConstantTokenEXT push = {};
			push.updateRange.stageFlags = VK_SHADER_STAGE_ALL;
			push.updateRange.offset = root.PushByteOffset;
			push.updateRange.size = sizeof(uint64_t);
			m_pushTokens.push_back(push);
			token.type = VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_DATA_EXT;
			token.data.pPushConstant = &m_pushTokens.back();
			offset += sizeof(uint64_t);
			break;
		}
		default:
			assert(!"Indirect argument is not implemented on Vulkan");
			return false;
		}
	}

	// Pointers alias vector storage, which may have moved while the tokens were filled.
	auto vertexIndex = 0u;
	auto indexIndex = 0u;
	auto pushIndex = 0u;
	for (auto i = 0u; i < numArguments; ++i)
	{
		switch (pArguments[i].Type)
		{
		case IndirectArgumentType::VERTEX_BUFFER_VIEW:
			m_tokens[i].data.pVertexBuffer = &m_vertexTokens[vertexIndex++];
			break;
		case IndirectArgumentType::INDEX_BUFFER_VIEW:
			m_tokens[i].data.pIndexBuffer = &m_indexTokens[indexIndex++];
			break;
		case IndirectArgumentType::CONSTANT:
		case IndirectArgumentType::INCREMENTING_CONSTANT:
		case IndirectArgumentType::CONSTANT_BUFFER_VIEW:
		case IndirectArgumentType::SHADER_RESOURCE_VIEW:
		case IndirectArgumentType::UNORDERED_ACCESS_VIEW:
			m_tokens[i].data.pPushConstant = &m_pushTokens[pushIndex++];
			break;
		default:
			break;
		}
	}

	m_traceRaysOnly = m_traceRays && numArguments == 1;
	const auto rayStages = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
		VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR |
		VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_CALLABLE_BIT_KHR;
	auto dgcRays = false;
	if (m_traceRays)
	{
		m_stages = rayStages;
		if (deviceGeneratedCommands)
		{
			const auto supported = m_pDevice->GetGeneratedCommandsProperties().supportedIndirectCommandsShaderStages & rayStages;
			dgcRays = (supported & VK_SHADER_STAGE_RAYGEN_BIT_KHR) != 0;
			if (dgcRays) m_stages = supported;
		}
		for (auto& push : m_pushTokens)
			push.updateRange.stageFlags = m_stages;
		if (!m_traceRaysOnly && !dgcRays)
		{
			assert(!"Indirect ray dispatch with other arguments needs device generated command ray stages");
			return false;
		}
	}
	else
	{
		m_stages = m_isDispatch && !graphics ? VK_SHADER_STAGE_COMPUTE_BIT :
			graphics && !m_isDispatch ? (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) : VK_SHADER_STAGE_ALL;
	}

	m_stride = byteStride ? byteStride : offset;

	// A rays-only signature uses vkCmdTraceRaysIndirect2KHR. DGC is kept for a count buffer
	// when the device can put raygen in an indirect-commands layout. vkd3d-proton does the same split.
	if (m_traceRaysOnly && !dgcRays) return true;

	assert(deviceGeneratedCommands);
	if (!deviceGeneratedCommands) return false;

	VkIndirectCommandsLayoutCreateInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_CREATE_INFO_EXT;
	info.shaderStages = m_stages;
	info.indirectStride = m_stride;
	info.pipelineLayout = VK_NULL_HANDLE;
	info.tokenCount = numArguments;
	info.pTokens = m_tokens.data();

	XUSG_VK_CHECK(m_pDevice->GetExtProcs().CreateIndirectCommandsLayout(m_pDevice->GetDevice(),
		&info, nullptr, &m_layout), cerr, false);
	m_pDevice->SetDebugName(VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_EXT, (uint64_t)m_layout, name);

	return true;
}

void CommandLayout_VK::Create(uint64_t nativeHandle, Device* pDevice)
{
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);

	m_layout = (VkIndirectCommandsLayoutEXT)nativeHandle;
}

uint64_t CommandLayout_VK::GetNativeHandle() const
{
	return (uint64_t)m_layout;
}

void* CommandLayout_VK::GetBackend()
{
	return this;
}

VkIndirectCommandsLayoutEXT CommandLayout_VK::GetLayout() const
{
	return m_layout;
}

VkShaderStageFlags CommandLayout_VK::GetShaderStages() const
{
	return m_stages;
}

uint32_t CommandLayout_VK::GetByteStride() const
{
	return m_stride;
}

bool CommandLayout_VK::IsDispatch() const
{
	return m_isDispatch;
}

bool CommandLayout_VK::IsTraceRays() const
{
	return m_traceRays;
}

bool CommandLayout_VK::IsTraceRaysOnly() const
{
	return m_traceRaysOnly;
}

//--------------------------------------------------------------------------------------

SwapChain_VK::SwapChain_VK() :
	m_pDevice(nullptr),
	m_queue(VK_NULL_HANDLE),
	m_family(0),
	m_surface(VK_NULL_HANDLE),
	m_swapChain(VK_NULL_HANDLE),
	m_format(VK_FORMAT_UNDEFINED),
	m_width(0),
	m_height(0),
	m_bufferCount(0),
	m_imageIndex(0),
	m_userIndex(0),
	m_allowTearing(false),
	m_acquired(false),
	m_lastAcquireSemaphore(VK_NULL_HANDLE),
	m_lastAcquireFence(VK_NULL_HANDLE),
	m_acquireSlot(0),
	m_blitPool(VK_NULL_HANDLE),
	m_blitSlot(0)
{
}

SwapChain_VK::~SwapChain_VK()
{
	if (m_queue != VK_NULL_HANDLE)
	{
		ClearQueueAcquireWait(m_queue);
		FlushQueuePendingPresent(m_queue);
		vkQueueWaitIdle(m_queue);
	}
	const auto device = m_pDevice ? m_pDevice->GetDevice() : VK_NULL_HANDLE;
	destroyBlitResources();
	destroyUserImages();
	for (const auto& slot : m_acquireSlots)
	{
		if (slot.Semaphore != VK_NULL_HANDLE) vkDestroySemaphore(device, slot.Semaphore, nullptr);
		if (slot.Fence != VK_NULL_HANDLE) vkDestroyFence(device, slot.Fence, nullptr);
	}
	for (const auto semaphore : m_releaseSemaphores)
		if (semaphore != VK_NULL_HANDLE) vkDestroySemaphore(device, semaphore, nullptr);
	if (m_swapChain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, m_swapChain, nullptr);
	if (m_surface != VK_NULL_HANDLE && m_pDevice) vkDestroySurfaceKHR(m_pDevice->GetInstance(), m_surface, nullptr);
}

bool SwapChain_VK::Create(void* pFactory, void* hWnd, void* pDevice, uint8_t bufferCount,
	uint32_t width, uint32_t height, Format format, SwapChainFlag flags, bool windowed)
{
	(void)pFactory;
	(void)windowed;
	const auto queue = static_cast<VkQueue>(pDevice);
	QueueNative native = {};
	const auto found = FindQueue(queue, native);
	assert(found);
	if (!found) return false;
	m_pDevice = native.Device;
	m_queue = native.Queue;
	m_family = native.Family;
	assert(m_pDevice);

	VkWin32SurfaceCreateInfoKHR surface = {};
	surface.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface.hwnd = static_cast<HWND>(hWnd);
	surface.hinstance = GetModuleHandle(nullptr);
	XUSG_VK_CHECK(vkCreateWin32SurfaceKHR(m_pDevice->GetInstance(), &surface, nullptr, &m_surface), cerr, false);

	VkBool32 supported = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_pDevice->GetPhysicalDevice(), m_family, m_surface, &supported);
	assert(supported);

	m_allowTearing = (flags & SwapChainFlag::ALLOW_TEARING) != SwapChainFlag::NONE;

	return createSwapchain(bufferCount, width, height, format, VK_NULL_HANDLE);
}

bool SwapChain_VK::Present(uint8_t syncInterval, PresentFlag flags)
{
	(void)syncInterval;
	(void)flags;

	// Queue blit+PresentKHR for Signal (vkd3d-style).
	QueuePendingPresent pending = {};
	pending.SwapChain = this;
	pending.UserIndex = m_userIndex;
	SetQueuePendingPresent(m_queue, pending);
	m_userIndex = (m_userIndex + 1) % (max)(static_cast<uint32_t>(m_bufferCount), 1u);

	return true;
}

bool SwapChain_VK::PresentEx(uint8_t syncInterval, PresentFlag flags, uint32_t dirtyRectsCount,
	const RectRange* pDirtyRects, const RectRange* pScrollRect, const Point* pScrollOffset)
{
	(void)dirtyRectsCount;
	(void)pDirtyRects;
	(void)pScrollRect;
	(void)pScrollOffset;

	return Present(syncInterval, flags);
}

bool SwapChain_VK::GetBuffer(uint8_t buffer, Resource* pResource) const
{
	assert(pResource);
	assert(buffer < m_userImages.size());
	const auto resource = AsVK(pResource);
	assert(resource);
	resource->WrapImage(m_pDevice->GetDevice(), m_userImages[buffer], m_format, m_width, m_height, false);

	return true;
}

uint32_t SwapChain_VK::ResizeBuffers(uint8_t bufferCount, uint32_t width, uint32_t height,
	Format format, SwapChainFlag flags)
{
	m_allowTearing = (flags & SwapChainFlag::ALLOW_TEARING) != SwapChainFlag::NONE;
	const auto old = m_swapChain;
	m_swapChain = VK_NULL_HANDLE;
	if (!createSwapchain(bufferCount, width, height, format == Format::UNKNOWN ? GetFormatFromVK(m_format) : format, old))
		return static_cast<uint32_t>(VK_ERROR_INITIALIZATION_FAILED);

	return VK_SUCCESS;
}

uint8_t SwapChain_VK::GetCurrentBackBufferIndex() const
{
	// DXGI-style user backbuffer index (no WSI acquire).
	return static_cast<uint8_t>(m_userIndex);
}

void SwapChain_VK::Create(uint64_t nativeHandle)
{
	m_swapChain = (VkSwapchainKHR)nativeHandle;
}

uint64_t SwapChain_VK::GetNativeHandle() const
{
	return (uint64_t)m_swapChain;
}

void* SwapChain_VK::GetBackend()
{
	return this;
}

VkImage SwapChain_VK::GetImage(uint8_t buffer) const
{
	assert(buffer < m_userImages.size());

	return m_userImages[buffer];
}

VkFormat SwapChain_VK::GetVkFormat() const
{
	return m_format;
}

uint32_t SwapChain_VK::GetWidth() const
{
	return m_width;
}

uint32_t SwapChain_VK::GetHeight() const
{
	return m_height;
}

bool SwapChain_VK::createSwapchain(uint8_t bufferCount, uint32_t width, uint32_t height, Format format, VkSwapchainKHR old)
{
	m_format = format == Format::UNKNOWN ? VK_FORMAT_B8G8R8A8_UNORM : GetVKFormat(format);
	m_width = width;
	m_height = height;
	m_bufferCount = bufferCount;
	const auto device = m_pDevice->GetDevice();
	const auto physical = m_pDevice->GetPhysicalDevice();

	if (m_queue != VK_NULL_HANDLE)
	{
		FlushQueuePendingPresent(m_queue);
		ClearQueueAcquireWait(m_queue);
		vkQueueWaitIdle(m_queue);
	}

	VkSurfaceCapabilitiesKHR caps = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, m_surface, &caps);

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = m_surface;
	// Prefer at least 3 WSI images; they are only held briefly for blit.
	info.minImageCount = (max)(3u, (max)(static_cast<uint32_t>(bufferCount), caps.minImageCount));
	if (caps.maxImageCount) info.minImageCount = (min)(info.minImageCount, caps.maxImageCount);
	info.imageFormat = m_format;
	info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	info.imageExtent = { width, height };
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	// Unlocked: IMMEDIATE over MAILBOX (blit owns pacing).
	info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	if (m_allowTearing)
	{
		uint32_t modeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical, m_surface, &modeCount, nullptr);
		vector<VkPresentModeKHR> modes(modeCount);
		if (modeCount && vkGetPhysicalDeviceSurfacePresentModesKHR(physical,
			m_surface, &modeCount, modes.data()) >= 0)
		{
			auto hasMailbox = false, hasImmediate = false;
			for (auto i = 0u; i < modeCount; ++i)
			{
				hasMailbox = hasMailbox || modes[i] == VK_PRESENT_MODE_MAILBOX_KHR;
				hasImmediate = hasImmediate || modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
			if (hasImmediate) info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			else if (hasMailbox) info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}
	info.clipped = VK_TRUE;
	info.oldSwapchain = old;

	XUSG_VK_CHECK(vkCreateSwapchainKHR(device, &info, nullptr, &m_swapChain), cerr, false);
	if (old != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, old, nullptr);

	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(device, m_swapChain, &imageCount, nullptr);
	m_images.resize(imageCount);
	vkGetSwapchainImagesKHR(device, m_swapChain, &imageCount, m_images.data());
	m_acquired = false;
	m_acquireSlot = 0;
	m_userIndex = 0;
	m_lastAcquireSemaphore = VK_NULL_HANDLE;
	m_lastAcquireFence = VK_NULL_HANDLE;
	if (m_queue != VK_NULL_HANDLE) ClearQueueAcquireWait(m_queue);
	for (const auto& slot : m_acquireSlots)
	{
		if (slot.Semaphore != VK_NULL_HANDLE) vkDestroySemaphore(device, slot.Semaphore, nullptr);
		if (slot.Fence != VK_NULL_HANDLE) vkDestroyFence(device, slot.Fence, nullptr);
	}
	m_acquireSlots.clear();
	constexpr uint32_t kAcquireRing = 8;
	m_acquireSlots.resize((max)(kAcquireRing, imageCount));
	for (auto& slot : m_acquireSlots)
	{
		VkSemaphoreCreateInfo semaphore = {};
		semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		XUSG_VK_CHECK(vkCreateSemaphore(device, &semaphore, nullptr, &slot.Semaphore), cerr, false);
		VkFenceCreateInfo fence = {};
		fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		XUSG_VK_CHECK(vkCreateFence(device, &fence, nullptr, &slot.Fence), cerr, false);
		slot.Submitted = false;
	}
	for (const auto semaphore : m_releaseSemaphores)
		if (semaphore != VK_NULL_HANDLE) vkDestroySemaphore(device, semaphore, nullptr);
	m_releaseSemaphores.assign(imageCount, VK_NULL_HANDLE);
	for (auto& semaphore : m_releaseSemaphores)
	{
		VkSemaphoreCreateInfo semInfo = {};
		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		XUSG_VK_CHECK(vkCreateSemaphore(device, &semInfo, nullptr, &semaphore), cerr, false);
	}

	XUSG_N_RETURN(createUserImages(), false);
	XUSG_N_RETURN(createBlitResources(), false);

	// The window is not shown yet when the swapchain is created from OnInit.
	// Acquiring here blocks forever inside vkAcquireNextImageKHR.
	return true;
}

bool SwapChain_VK::createUserImages()
{
	destroyUserImages();
	const auto device = m_pDevice->GetDevice();
	m_userImages.resize(m_bufferCount);
	m_userMemory.resize(m_bufferCount);
	for (auto i = 0u; i < m_bufferCount; ++i)
	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = m_format;
		info.extent = { m_width, m_height, 1 };
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		XUSG_VK_CHECK(vkCreateImage(device, &info, nullptr, &m_userImages[i]), cerr, false);

		VkMemoryRequirements reqs = {};
		vkGetImageMemoryRequirements(device, m_userImages[i], &reqs);
		VkMemoryAllocateInfo alloc = {};
		alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc.allocationSize = reqs.size;
		alloc.memoryTypeIndex = m_pDevice->FindMemoryType(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		assert(alloc.memoryTypeIndex != UINT32_MAX);
		XUSG_VK_CHECK(vkAllocateMemory(device, &alloc, nullptr, &m_userMemory[i]), cerr, false);
		XUSG_VK_CHECK(vkBindImageMemory(device, m_userImages[i], m_userMemory[i], 0), cerr, false);
	}

	return true;
}

void SwapChain_VK::destroyUserImages()
{
	const auto device = m_pDevice ? m_pDevice->GetDevice() : VK_NULL_HANDLE;
	for (auto i = 0u; i < m_userImages.size(); ++i)
	{
		if (m_userImages[i] != VK_NULL_HANDLE) vkDestroyImage(device, m_userImages[i], nullptr);
		if (i < m_userMemory.size() && m_userMemory[i] != VK_NULL_HANDLE)
			vkFreeMemory(device, m_userMemory[i], nullptr);
	}
	m_userImages.clear();
	m_userMemory.clear();
}

bool SwapChain_VK::createBlitResources()
{
	destroyBlitResources();
	const auto device = m_pDevice->GetDevice();
	VkCommandPoolCreateInfo pool = {};
	pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool.queueFamilyIndex = m_family;
	XUSG_VK_CHECK(vkCreateCommandPool(device, &pool, nullptr, &m_blitPool), cerr, false);

	m_blitBuffers.resize((max)(static_cast<size_t>(m_bufferCount), m_acquireSlots.size()));
	VkCommandBufferAllocateInfo alloc = {};
	alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc.commandPool = m_blitPool;
	alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc.commandBufferCount = static_cast<uint32_t>(m_blitBuffers.size());
	XUSG_VK_CHECK(vkAllocateCommandBuffers(device, &alloc, m_blitBuffers.data()), cerr, false);
	m_blitSlot = 0;

	return true;
}

void SwapChain_VK::destroyBlitResources()
{
	const auto device = m_pDevice ? m_pDevice->GetDevice() : VK_NULL_HANDLE;
	if (m_blitPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(device, m_blitPool, nullptr);
		m_blitPool = VK_NULL_HANDLE;
	}
	m_blitBuffers.clear();
	m_blitSlot = 0;
}

bool SwapChain_VK::acquire() const
{
	if (m_acquired) return true;
	if (m_acquireSlots.empty()) return false;

	auto& slot = m_acquireSlots[m_acquireSlot];
	const auto device = m_pDevice->GetDevice();
	if (slot.Submitted)
	{
		if (vkGetFenceStatus(device, slot.Fence) != VK_SUCCESS)
			vkWaitForFences(device, 1, &slot.Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &slot.Fence);
		slot.Submitted = false;
	}

	uint32_t index = 0;
	const auto result = vkAcquireNextImageKHR(device, m_swapChain, UINT64_MAX, slot.Semaphore, VK_NULL_HANDLE, &index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) return false;
	if (result < 0)
	{
		cerr << "VkResult " << result << endl;
		assert(result >= 0);

		return false;
	}
	slot.Submitted = true;
	m_lastAcquireSemaphore = slot.Semaphore;
	m_lastAcquireFence = slot.Fence;
	m_acquireSlot = (m_acquireSlot + 1) % static_cast<uint32_t>(m_acquireSlots.size());
	m_imageIndex = index;
	m_acquired = true;

	return true;
}

bool SwapChain_VK::flushPresent(uint32_t userIndex)
{
	assert(userIndex < m_userImages.size());
	if (!acquire()) return false;
	assert(m_imageIndex < m_images.size());
	assert(m_imageIndex < m_releaseSemaphores.size());
	if (m_blitBuffers.empty()) return false;

	const auto blitCb = m_blitBuffers[m_blitSlot];
	m_blitSlot = (m_blitSlot + 1) % static_cast<uint32_t>(m_blitBuffers.size());
	XUSG_VK_CHECK(vkResetCommandBuffer(blitCb, 0), cerr, false);

	VkCommandBufferBeginInfo begin = {};
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	XUSG_VK_CHECK(vkBeginCommandBuffer(blitCb, &begin), cerr, false);

	// vkd3d blit path: UNDEFINED->DST + COLOR->SRC, BlitImage, DST->PRESENT + SRC->COLOR.
	VkImageMemoryBarrier2 barriers[2] = {};
	barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	barriers[0].dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barriers[0].image = m_images[m_imageIndex];
	barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barriers[0].subresourceRange.levelCount = 1;
	barriers[0].subresourceRange.layerCount = 1;

	barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	barriers[1].dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	barriers[1].dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
	barriers[1].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barriers[1].image = m_userImages[userIndex];
	barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barriers[1].subresourceRange.levelCount = 1;
	barriers[1].subresourceRange.layerCount = 1;

	VkDependencyInfo dep = {};
	dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dep.imageMemoryBarrierCount = 2;
	dep.pImageMemoryBarriers = barriers;
	vkCmdPipelineBarrier2(blitCb, &dep);

	VkImageBlit blit = {};
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.layerCount = 1;
	blit.dstSubresource = blit.srcSubresource;
	blit.srcOffsets[1] = { static_cast<int32_t>(m_width), static_cast<int32_t>(m_height), 1 };
	blit.dstOffsets[1] = blit.srcOffsets[1];
	vkCmdBlitImage(blitCb, m_userImages[userIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		m_images[m_imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);

	barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	barriers[0].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_NONE;
	barriers[0].dstAccessMask = VK_ACCESS_2_NONE;
	barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barriers[0].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	barriers[1].srcAccessMask = VK_ACCESS_2_NONE;
	barriers[1].dstStageMask = VK_PIPELINE_STAGE_2_NONE;
	barriers[1].dstAccessMask = VK_ACCESS_2_NONE;
	barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barriers[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	vkCmdPipelineBarrier2(blitCb, &dep);
	XUSG_VK_CHECK(vkEndCommandBuffer(blitCb), cerr, false);

	VkSemaphoreSubmitInfo wait = {};
	wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait.semaphore = m_lastAcquireSemaphore;
	wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT;

	VkSemaphoreSubmitInfo signal = {};
	signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signal.semaphore = m_releaseSemaphores[m_imageIndex];
	signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	VkCommandBufferSubmitInfo cmd = {};
	cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	cmd.commandBuffer = blitCb;

	VkSubmitInfo2 submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit.waitSemaphoreInfoCount = 1;
	submit.pWaitSemaphoreInfos = &wait;
	submit.commandBufferInfoCount = 1;
	submit.pCommandBufferInfos = &cmd;
	submit.signalSemaphoreInfoCount = 1;
	submit.pSignalSemaphoreInfos = &signal;
	XUSG_N_RETURN(m_pDevice->SubmitQueue(m_queue, submit, m_lastAcquireFence,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr), false);

	VkPresentInfoKHR present = {};
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = &m_releaseSemaphores[m_imageIndex];
	present.swapchainCount = 1;
	present.pSwapchains = &m_swapChain;
	present.pImageIndices = &m_imageIndex;
	const auto result = vkQueuePresentKHR(m_queue, &present);
	m_acquired = false;
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return true;
	XUSG_VK_CHECK(result, cerr, false);

	return true;
}

void XUSG::GetVKBlobData(const Blob& blob, const void** ppData, size_t* pSize)
{
	const auto pBlob = static_cast<const Blob_VK*>(blob);
	if (ppData) *ppData = pBlob && !pBlob->Data.empty() ? pBlob->Data.data() : nullptr;
	if (pSize) *pSize = pBlob ? pBlob->Data.size() : 0;
}

Blob XUSG::GetVKPipelineCache(Pipeline pipeline)
{
	const auto pPipeline = static_cast<Pipeline_VK*>(pipeline);

	return pPipeline ? &pPipeline->Cache : nullptr;
}

void XUSG::RetainVKPipeline(Pipeline_VK* pPipeline)
{
	if (pPipeline) ++pPipeline->Refs;
}

void XUSG::ReleaseVKPipeline(VkDevice device, Pipeline_VK* pPipeline)
{
	if (!pPipeline) return;
	if (--pPipeline->Refs) return;
	if (pPipeline->Handle != VK_NULL_HANDLE) vkDestroyPipeline(device, pPipeline->Handle, nullptr);
	delete pPipeline;
}
