//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSG_VK.h"
#include "XUSGResource_VK.h"
#include "XUSGEnum_VK.h"

#include <map>

using namespace std;
using namespace XUSG;

namespace
{
	mutex g_queueMutex;
	unordered_map<VkQueue, QueueNative> g_queues;
	unordered_map<VkQueue, uint32_t> g_queueRefs;
	unordered_map<VkQueue, VkSemaphore> g_acquireWaits;

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

	mutex g_bufferMutex;
	map<VkDeviceAddress, BufferNative> g_buffers;
	unordered_map<VkBuffer, VkDeviceAddress> g_bufferAddresses;

	bool HasExtension(const vector<VkExtensionProperties>& props, const char* name)
	{
		for (const auto& prop : props)
			if (strcmp(prop.extensionName, name) == 0) return true;

		return false;
	}
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

void XUSG::SetQueueAcquireWait(VkQueue queue, VkSemaphore semaphore)
{
	lock_guard<mutex> lock(g_queueMutex);
	g_acquireWaits[queue] = semaphore;
}

bool XUSG::TakeQueueAcquireWait(VkQueue queue, VkSemaphore& semaphore)
{
	lock_guard<mutex> lock(g_queueMutex);
	const auto it = g_acquireWaits.find(queue);
	if (it == g_acquireWaits.cend() || !it->second) return false;

	semaphore = it->second;
	it->second = VK_NULL_HANDLE;

	return true;
}

void XUSG::ClearQueueAcquireWait(VkQueue queue)
{
	lock_guard<mutex> lock(g_queueMutex);
	g_acquireWaits.erase(queue);
}

void XUSG::RegisterBuffer(const BufferNative& buffer)
{
	if (!buffer.Buffer || !buffer.Address) return;

	lock_guard<mutex> lock(g_bufferMutex);
	const auto previous = g_bufferAddresses.find(buffer.Buffer);
	if (previous != g_bufferAddresses.cend()) g_buffers.erase(previous->second);

	g_bufferAddresses[buffer.Buffer] = buffer.Address;
	g_buffers[buffer.Address] = buffer;
}

void XUSG::UnregisterBuffer(VkBuffer buffer)
{
	if (!buffer) return;

	lock_guard<mutex> lock(g_bufferMutex);
	const auto it = g_bufferAddresses.find(buffer);
	if (it == g_bufferAddresses.cend()) return;

	g_buffers.erase(it->second);
	g_bufferAddresses.erase(it);
}

bool XUSG::FindBuffer(VkDeviceAddress address, BufferNative& native)
{
	lock_guard<mutex> lock(g_bufferMutex);
	auto it = g_buffers.upper_bound(address);
	if (it == g_buffers.begin()) return false;

	--it;
	if (address < it->second.Address || address >= it->second.Address + it->second.Size) return false;

	native = it->second;

	return true;
}

PipelineLayoutObject::PipelineLayoutObject() :
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
	m_meshShader(false),
	m_fragmentShadingRate(false),
	m_conditionalRendering(false),
	m_conservativeRaster(false),
	m_ownsInstance(false),
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
	m_sharingCount(0)
{
}

Device_VK::~Device_VK()
{
	if (m_device) vkDestroyDevice(m_device, nullptr);
	if (m_ownsInstance && m_instance) vkDestroyInstance(m_instance, nullptr);
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
	const CommandAllocator* pCommandAllocator, const Pipeline& pipeline, const wchar_t* name)
{
	return pCommandList->Create(this, nodeMask, type, pCommandAllocator, pipeline, name);
}

bool Device_VK::GetFence(Fence* pFence, uint64_t initialValue, FenceFlag flags, const wchar_t* name)
{
	return pFence->Create(this, initialValue, flags, name);
}

uint32_t Device_VK::Create(void* pAdapter, uint32_t minFeatureLevel, const wchar_t* name)
{
	// minFeatureLevel is a D3D feature level on the DX12 path. Vulkan requires 1.2+ and the
	// extensions asserted below; the feature-level argument is not a Vulkan version.
	(void)minFeatureLevel;

	XUSG_N_RETURN(createInstance(), static_cast<uint32_t>(VK_ERROR_INITIALIZATION_FAILED));
	XUSG_N_RETURN(pickPhysicalDevice(static_cast<VkPhysicalDevice>(pAdapter)), static_cast<uint32_t>(VK_ERROR_INITIALIZATION_FAILED));
	XUSG_N_RETURN(createDevice(), static_cast<uint32_t>(VK_ERROR_INITIALIZATION_FAILED));

	if (name) SetDebugName(VK_OBJECT_TYPE_DEVICE, reinterpret_cast<uint64_t>(m_device), name);

	return VK_SUCCESS;
}

uint32_t Device_VK::GetDeviceRemovedReason() const
{
	return m_removedReason;
}

void Device_VK::Create(void* pHandle, const wchar_t* name)
{
	m_device = static_cast<VkDevice>(pHandle);
	if (name) SetDebugName(VK_OBJECT_TYPE_DEVICE, reinterpret_cast<uint64_t>(m_device), name);
}

void* Device_VK::GetHandle() const
{
	return m_device;
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

bool Device_VK::HasDebugUtils() const
{
	return m_debugUtils;
}

bool Device_VK::HasCustomBorderColor() const
{
	return m_customBorderColor;
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

bool Device_VK::HasConservativeRasterization() const
{
	return m_conservativeRaster;
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
	if (!m_debugUtils || !name || !m_device) return;

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
	vkEnumerateInstanceVersion(&loaderVersion);
	m_apiVersion = loaderVersion >= VK_API_VERSION_1_3 ? VK_API_VERSION_1_3 : VK_API_VERSION_1_2;
	assert(loaderVersion >= VK_API_VERSION_1_2);

	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	vector<VkExtensionProperties> exts(extCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.data());

	vector<const char*> enabled;
	enabled.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	enabled.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	m_debugUtils = HasExtension(exts, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	if (m_debugUtils) enabled.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	VkApplicationInfo app = {};
	app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pApplicationName = "XUSG";
	app.apiVersion = m_apiVersion;

	VkInstanceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info.pApplicationInfo = &app;
	info.enabledExtensionCount = static_cast<uint32_t>(enabled.size());
	info.ppEnabledExtensionNames = enabled.data();

	XUSG_VK_CHECK(vkCreateInstance(&info, nullptr, &m_instance), cerr, false);
	m_ownsInstance = true;

	return true;
}

bool Device_VK::pickPhysicalDevice(VkPhysicalDevice requested)
{
	if (requested)
	{
		m_physicalDevice = requested;

		return true;
	}

	uint32_t count = 0;
	vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
	assert(count);
	vector<VkPhysicalDevice> devices(count);
	vkEnumeratePhysicalDevices(m_instance, &count, devices.data());
	m_physicalDevice = devices[0];

	return m_physicalDevice != VK_NULL_HANDLE;
}

bool Device_VK::createDevice()
{
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
	assert(graphicsFamily != UINT32_MAX);
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
		assert(supported && name);

		return supported;
	};

	vector<const char*> enabled;
	XUSG_N_RETURN(require(VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME), false);
	XUSG_N_RETURN(require(VK_EXT_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME), false);
	XUSG_N_RETURN(require(VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME), false);
	XUSG_N_RETURN(require(VK_KHR_SWAPCHAIN_EXTENSION_NAME), false);
	auto robustName = VK_KHR_ROBUSTNESS_2_EXTENSION_NAME;
	if (!HasExtension(exts, robustName)) robustName = VK_EXT_ROBUSTNESS_2_EXTENSION_NAME;
	XUSG_N_RETURN(require(robustName), false);
	auto divisorName = VK_KHR_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME;
	if (!HasExtension(exts, divisorName)) divisorName = VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME;
	XUSG_N_RETURN(require(divisorName), false);
	XUSG_N_RETURN(require(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME), false);
	XUSG_N_RETURN(require(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME), false);
	XUSG_N_RETURN(require(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME), false);
	XUSG_N_RETURN(require(VK_KHR_MAINTENANCE_5_EXTENSION_NAME), false);
	enabled.push_back(VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME);
	enabled.push_back(VK_EXT_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME);
	enabled.push_back(VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME);
	enabled.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	enabled.push_back(robustName);
	enabled.push_back(divisorName);
	enabled.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
	enabled.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	enabled.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
	enabled.push_back(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);

	const auto coreDynamicRendering = m_apiVersion >= VK_API_VERSION_1_3;

	m_meshShader = HasExtension(exts, VK_EXT_MESH_SHADER_EXTENSION_NAME);
	if (m_meshShader) enabled.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	m_fragmentShadingRate = HasExtension(exts, VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
	if (m_fragmentShadingRate) enabled.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
	m_conditionalRendering = HasExtension(exts, VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);
	if (m_conditionalRendering) enabled.push_back(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);
	m_conservativeRaster = HasExtension(exts, VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);
	if (m_conservativeRaster) enabled.push_back(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);
	m_customBorderColor = HasExtension(exts, VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);
	if (m_customBorderColor) enabled.push_back(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);

	VkPhysicalDeviceProperties props = {};
	vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
	if (props.apiVersion < m_apiVersion) m_apiVersion = props.apiVersion;

	m_heapProps = {};
	m_heapProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;
	m_dgcProps = {};
	m_dgcProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_EXT;
	m_dgcProps.pNext = &m_heapProps;

	VkPhysicalDeviceDescriptorHeapFeaturesEXT heapFeatures = {};
	heapFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT;
	VkPhysicalDeviceDeviceGeneratedCommandsFeaturesEXT dgcFeatures = {};
	dgcFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_EXT;
	dgcFeatures.pNext = &heapFeatures;
	VkPhysicalDeviceRobustness2FeaturesKHR robust = {};
	robust.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR;
	VkPhysicalDeviceVertexAttributeDivisorFeatures divisor = {};
	divisor.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES;
	divisor.pNext = &robust;
	VkPhysicalDeviceConditionalRenderingFeaturesEXT conditional = {};
	conditional.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
	conditional.pNext = &divisor;

	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
	dynamicState.pNext = m_conditionalRendering ? static_cast<void*>(&conditional) : static_cast<void*>(&divisor);

	VkPhysicalDeviceMaintenance5Features maintenance5 = {};
	maintenance5.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES;
	maintenance5.pNext = &dynamicState;

	VkPhysicalDeviceShaderUntypedPointersFeaturesKHR untyped = {};
	untyped.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR;
	untyped.pNext = &maintenance5;

	VkPhysicalDeviceFeatures2 features = {};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features.pNext = &untyped;
	dgcFeatures.pNext = &heapFeatures;
	robust.pNext = &dgcFeatures;
	vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features);
	assert(heapFeatures.descriptorHeap);
	assert(dgcFeatures.deviceGeneratedCommands);
	assert(untyped.shaderUntypedPointers);
	assert(robust.nullDescriptor);
	assert(divisor.vertexAttributeInstanceRateDivisor);
	assert(dynamicState.extendedDynamicState);
	assert(maintenance5.maintenance5);
	if (m_conditionalRendering && !conditional.conditionalRendering) m_conditionalRendering = false;

	VkPhysicalDeviceProperties2 props2 = {};
	props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	props2.pNext = &m_dgcProps;
	vkGetPhysicalDeviceProperties2(m_physicalDevice, &props2);

	const auto imageStride = static_cast<uint32_t>(m_heapProps.imageDescriptorSize);
	const auto bufferStride = static_cast<uint32_t>(m_heapProps.bufferDescriptorSize);
	m_resourceStride = (max)(imageStride, bufferStride);
	m_samplerStride = static_cast<uint32_t>(m_heapProps.samplerDescriptorSize);
	assert(m_resourceStride);
	assert(m_samplerStride);

	VkPhysicalDeviceShaderUntypedPointersFeaturesKHR untypedEnable = {};
	untypedEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR;
	untypedEnable.shaderUntypedPointers = VK_TRUE;

	VkPhysicalDeviceDescriptorHeapFeaturesEXT heapEnable = {};
	heapEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT;
	heapEnable.pNext = &untypedEnable;
	heapEnable.descriptorHeap = VK_TRUE;

	VkPhysicalDeviceDeviceGeneratedCommandsFeaturesEXT dgcEnable = {};
	dgcEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_EXT;
	dgcEnable.pNext = &heapEnable;
	dgcEnable.deviceGeneratedCommands = VK_TRUE;

	VkPhysicalDeviceVulkan12Features features12 = {};
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.pNext = &dgcEnable;
	features12.bufferDeviceAddress = VK_TRUE;
	features12.timelineSemaphore = VK_TRUE;

	void* featureChain = &features12;
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateEnable = {};
	dynamicStateEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
	dynamicStateEnable.extendedDynamicState = VK_TRUE;

	VkPhysicalDeviceVulkan13Features features13 = {};
	VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering = {};
	VkPhysicalDeviceSynchronization2Features sync2 = {};
	if (coreDynamicRendering)
	{
		features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		features13.pNext = &features12;
		features13.dynamicRendering = VK_TRUE;
		features13.synchronization2 = VK_TRUE;
		dynamicStateEnable.pNext = &features13;
		featureChain = &dynamicStateEnable;
	}
	else
	{
		dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		dynamicRendering.pNext = &features12;
		dynamicRendering.dynamicRendering = VK_TRUE;
		sync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		sync2.pNext = &dynamicRendering;
		sync2.synchronization2 = VK_TRUE;
		dynamicStateEnable.pNext = &sync2;
		featureChain = &dynamicStateEnable;
	}

	VkPhysicalDeviceMeshShaderFeaturesEXT mesh = {};
	if (m_meshShader)
	{
		mesh.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
		mesh.pNext = featureChain;
		mesh.meshShader = VK_TRUE;
		mesh.taskShader = VK_TRUE;
		featureChain = &mesh;
	}

	VkPhysicalDeviceFragmentShadingRateFeaturesKHR shadingRate = {};
	if (m_fragmentShadingRate)
	{
		shadingRate.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
		shadingRate.pNext = featureChain;
		shadingRate.pipelineFragmentShadingRate = VK_TRUE;
		shadingRate.primitiveFragmentShadingRate = VK_TRUE;
		shadingRate.attachmentFragmentShadingRate = VK_TRUE;
		featureChain = &shadingRate;
	}

	VkPhysicalDeviceCustomBorderColorFeaturesEXT border = {};
	if (m_customBorderColor)
	{
		border.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
		border.pNext = featureChain;
		border.customBorderColors = VK_TRUE;
		featureChain = &border;
	}

	VkPhysicalDeviceRobustness2FeaturesKHR robustEnable = {};
	robustEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR;
	robustEnable.pNext = featureChain;
	robustEnable.nullDescriptor = VK_TRUE;
	featureChain = &robustEnable;

	VkPhysicalDeviceVertexAttributeDivisorFeatures divisorEnable = {};
	divisorEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES;
	divisorEnable.pNext = featureChain;
	divisorEnable.vertexAttributeInstanceRateDivisor = VK_TRUE;
	featureChain = &divisorEnable;

	VkPhysicalDeviceConditionalRenderingFeaturesEXT conditionalEnable = {};
	if (m_conditionalRendering)
	{
		conditionalEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
		conditionalEnable.pNext = featureChain;
		conditionalEnable.conditionalRendering = VK_TRUE;
		featureChain = &conditionalEnable;
	}

	VkPhysicalDeviceMaintenance5Features maintenance5Enable = {};
	maintenance5Enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES;
	maintenance5Enable.pNext = featureChain;
	maintenance5Enable.maintenance5 = VK_TRUE;
	featureChain = &maintenance5Enable;

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
	assert(supported.samplerAnisotropy);
	assert(supported.independentBlend);
	assert(supported.fillModeNonSolid);

	VkPhysicalDeviceFeatures enabledFeatures = {};
	enabledFeatures.samplerAnisotropy = supported.samplerAnisotropy;
	enabledFeatures.geometryShader = supported.geometryShader;
	enabledFeatures.tessellationShader = supported.tessellationShader;
	enabledFeatures.fillModeNonSolid = supported.fillModeNonSolid;
	enabledFeatures.independentBlend = supported.independentBlend;
	enabledFeatures.multiDrawIndirect = supported.multiDrawIndirect;
	enabledFeatures.depthClamp = supported.depthClamp;
	enabledFeatures.fragmentStoresAndAtomics = supported.fragmentStoresAndAtomics;
	enabledFeatures.shaderInt64 = supported.shaderInt64;
	enabledFeatures.shaderStorageImageWriteWithoutFormat = supported.shaderStorageImageWriteWithoutFormat;

	VkDeviceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	info.pNext = featureChain;
	info.queueCreateInfoCount = m_poolCount;
	info.pQueueCreateInfos = queueInfos;
	info.enabledExtensionCount = static_cast<uint32_t>(enabled.size());
	info.ppEnabledExtensionNames = enabled.data();
	info.pEnabledFeatures = &enabledFeatures;

	XUSG_VK_CHECK(vkCreateDevice(m_physicalDevice, &info, nullptr, &m_device), cerr, false);

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
	m_ext.CreateIndirectCommandsLayout = reinterpret_cast<PFN_vkCreateIndirectCommandsLayoutEXT>(
		vkGetDeviceProcAddr(m_device, "vkCreateIndirectCommandsLayoutEXT"));
	m_ext.DestroyIndirectCommandsLayout = reinterpret_cast<PFN_vkDestroyIndirectCommandsLayoutEXT>(
		vkGetDeviceProcAddr(m_device, "vkDestroyIndirectCommandsLayoutEXT"));
	m_ext.BindSamplerHeap = reinterpret_cast<PFN_vkCmdBindSamplerHeapEXT>(vkGetDeviceProcAddr(m_device, "vkCmdBindSamplerHeapEXT"));
	m_ext.BindResourceHeap = reinterpret_cast<PFN_vkCmdBindResourceHeapEXT>(vkGetDeviceProcAddr(m_device, "vkCmdBindResourceHeapEXT"));
	m_ext.PushData = reinterpret_cast<PFN_vkCmdPushDataEXT>(vkGetDeviceProcAddr(m_device, "vkCmdPushDataEXT"));
	m_ext.GetGeneratedCommandsMemoryRequirements = reinterpret_cast<PFN_vkGetGeneratedCommandsMemoryRequirementsEXT>(
		vkGetDeviceProcAddr(m_device, "vkGetGeneratedCommandsMemoryRequirementsEXT"));
	m_ext.PreprocessGeneratedCommands = reinterpret_cast<PFN_vkCmdPreprocessGeneratedCommandsEXT>(
		vkGetDeviceProcAddr(m_device, "vkCmdPreprocessGeneratedCommandsEXT"));
	m_ext.ExecuteGeneratedCommands = reinterpret_cast<PFN_vkCmdExecuteGeneratedCommandsEXT>(
		vkGetDeviceProcAddr(m_device, "vkCmdExecuteGeneratedCommandsEXT"));
	m_ext.WriteSamplerDescriptors = reinterpret_cast<PFN_vkWriteSamplerDescriptorsEXT>(
		vkGetDeviceProcAddr(m_device, "vkWriteSamplerDescriptorsEXT"));
	m_ext.WriteResourceDescriptors = reinterpret_cast<PFN_vkWriteResourceDescriptorsEXT>(
		vkGetDeviceProcAddr(m_device, "vkWriteResourceDescriptorsEXT"));
	assert(m_ext.CreateIndirectCommandsLayout);
	assert(m_ext.DestroyIndirectCommandsLayout);
	assert(m_ext.BindSamplerHeap);
	assert(m_ext.BindResourceHeap);
	assert(m_ext.PushData);
	assert(m_ext.GetGeneratedCommandsMemoryRequirements);
	assert(m_ext.PreprocessGeneratedCommands);
	assert(m_ext.ExecuteGeneratedCommands);
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

	return true;
}

const Device_VK::ExtProcs& Device_VK::GetExtProcs() const
{
	return m_ext;
}

//--------------------------------------------------------------------------------------

Fence_VK::Fence_VK() :
	m_device(VK_NULL_HANDLE),
	m_semaphore(VK_NULL_HANDLE),
	m_pDevice(nullptr)
{
}

Fence_VK::~Fence_VK()
{
	if (m_semaphore) vkDestroySemaphore(m_device, m_semaphore, nullptr);
}

bool Fence_VK::Create(const Device* pDevice, uint64_t initialValue, FenceFlag flags, const wchar_t* name)
{
	(void)flags;
	const auto pDeviceVK = dynamic_cast<const Device_VK*>(pDevice);
	assert(pDeviceVK);
	m_device = pDeviceVK->GetDevice();
	m_pDevice = pDevice;

	VkSemaphoreTypeCreateInfo type = {};
	type.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	type.initialValue = initialValue;

	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	info.pNext = &type;

	XUSG_VK_CHECK(vkCreateSemaphore(m_device, &info, nullptr, &m_semaphore), cerr, false);
	pDeviceVK->SetDebugName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(m_semaphore), name);

	return true;
}

bool Fence_VK::SetEventOnCompletion(uint64_t value, void* hEvent)
{
	const auto device = m_device;
	const auto semaphore = m_semaphore;
	thread([device, semaphore, value, hEvent]()
	{
		VkSemaphoreWaitInfo wait = {};
		wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		wait.semaphoreCount = 1;
		wait.pSemaphores = &semaphore;
		wait.pValues = &value;
		vkWaitSemaphores(device, &wait, UINT64_MAX);
		if (hEvent) XUSG_SIGNAL_EVENT(hEvent);
	}).detach();

	return true;
}

bool Fence_VK::Signal(uint64_t value)
{
	VkSemaphoreSignalInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
	info.semaphore = m_semaphore;
	info.value = value;
	XUSG_VK_CHECK(vkSignalSemaphore(m_device, &info), cerr, false);

	return true;
}

uint64_t Fence_VK::GetCompletedValue() const
{
	uint64_t value = 0;
	vkGetSemaphoreCounterValue(m_device, m_semaphore, &value);

	return value;
}

void Fence_VK::Create(void* pHandle, const wchar_t* name)
{
	m_semaphore = static_cast<VkSemaphore>(pHandle);
	(void)name;
}

void* Fence_VK::GetHandle() const
{
	return m_semaphore;
}

VkSemaphore Fence_VK::GetSemaphore() const
{
	return m_semaphore;
}

//--------------------------------------------------------------------------------------

CommandLayout_VK::CommandLayout_VK() :
	m_device(VK_NULL_HANDLE),
	m_pDevice(nullptr),
	m_layout(VK_NULL_HANDLE),
	m_stages(VK_SHADER_STAGE_ALL),
	m_stride(0),
	m_isDispatch(false)
{
}

CommandLayout_VK::~CommandLayout_VK()
{
	if (m_layout && m_pDevice) m_pDevice->GetExtProcs().DestroyIndirectCommandsLayout(m_device, m_layout, nullptr);
}

bool CommandLayout_VK::Create(const Device* pDevice, uint32_t byteStride, uint32_t numArguments,
	const IndirectArgument* pArguments, const PipelineLayout& pipelineLayout, uint32_t nodeMask, const wchar_t* name)
{
	(void)nodeMask;
	const auto pDeviceVK = dynamic_cast<const Device_VK*>(pDevice);
	assert(pDeviceVK);
	m_device = pDeviceVK->GetDevice();
	m_pDevice = pDeviceVK;

	const auto pLayout = static_cast<const PipelineLayoutObject*>(pipelineLayout);
	m_tokens.resize(numArguments);
	m_vertexTokens.clear();
	m_indexTokens.clear();
	m_pushTokens.clear();

	auto offset = 0u;
	m_isDispatch = false;
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

	m_stages = m_isDispatch && !graphics ? VK_SHADER_STAGE_COMPUTE_BIT :
		graphics && !m_isDispatch ? (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) : VK_SHADER_STAGE_ALL;

	VkIndirectCommandsLayoutCreateInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_CREATE_INFO_EXT;
	info.shaderStages = m_stages;
	m_stride = byteStride ? byteStride : offset;
	info.indirectStride = m_stride;
	info.pipelineLayout = VK_NULL_HANDLE;
	info.tokenCount = numArguments;
	info.pTokens = m_tokens.data();

	XUSG_VK_CHECK(m_pDevice->GetExtProcs().CreateIndirectCommandsLayout(m_device, &info, nullptr, &m_layout), cerr, false);
	pDeviceVK->SetDebugName(VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_EXT, reinterpret_cast<uint64_t>(m_layout), name);

	return true;
}

void CommandLayout_VK::Create(void* pHandle, const wchar_t* name)
{
	m_layout = static_cast<VkIndirectCommandsLayoutEXT>(pHandle);
	(void)name;
}

void* CommandLayout_VK::GetHandle() const
{
	return m_layout;
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

//--------------------------------------------------------------------------------------

SwapChain_VK::SwapChain_VK() :
	m_instance(VK_NULL_HANDLE),
	m_physicalDevice(VK_NULL_HANDLE),
	m_device(VK_NULL_HANDLE),
	m_queue(VK_NULL_HANDLE),
	m_family(0),
	m_surface(VK_NULL_HANDLE),
	m_swapChain(VK_NULL_HANDLE),
	m_format(VK_FORMAT_UNDEFINED),
	m_width(0),
	m_height(0),
	m_bufferCount(0),
	m_imageIndex(0),
	m_allowTearing(false),
	m_acquired(false),
	m_acquireSemaphore(VK_NULL_HANDLE)
{
}

SwapChain_VK::~SwapChain_VK()
{
	if (m_queue)
	{
		ClearQueueAcquireWait(m_queue);
		vkQueueWaitIdle(m_queue);
	}
	if (m_acquireSemaphore) vkDestroySemaphore(m_device, m_acquireSemaphore, nullptr);
	for (const auto semaphore : m_renderSemaphores)
		if (semaphore) vkDestroySemaphore(m_device, semaphore, nullptr);
	if (m_swapChain) vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
	if (m_surface) vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

bool SwapChain_VK::Create(void* pFactory, void* hWnd, void* pDevice, uint8_t bufferCount,
	uint32_t width, uint32_t height, Format format, SwapChainFlag flags, bool windowed)
{
	(void)pFactory;
	(void)windowed;
	const auto queue = static_cast<VkQueue>(pDevice);
	QueueNative native = {};
	assert(FindQueue(queue, native));
	m_instance = native.Instance;
	m_physicalDevice = native.PhysicalDevice;
	m_device = native.Device;
	m_queue = native.Queue;
	m_family = native.Family;

	VkWin32SurfaceCreateInfoKHR surface = {};
	surface.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface.hwnd = static_cast<HWND>(hWnd);
	surface.hinstance = GetModuleHandle(nullptr);
	XUSG_VK_CHECK(vkCreateWin32SurfaceKHR(m_instance, &surface, nullptr, &m_surface), cerr, false);

	VkBool32 supported = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, m_family, m_surface, &supported);
	assert(supported);

	m_allowTearing = (flags & SwapChainFlag::ALLOW_TEARING) != SwapChainFlag::NONE;

	return createSwapchain(bufferCount, width, height, format, VK_NULL_HANDLE);
}

bool SwapChain_VK::createSwapchain(uint8_t bufferCount, uint32_t width, uint32_t height, Format format, VkSwapchainKHR old)
{
	m_format = format == Format::UNKNOWN ? VK_FORMAT_B8G8R8A8_UNORM : GetVKFormat(format);
	m_width = width;
	m_height = height;
	m_bufferCount = bufferCount;

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = m_surface;
	info.minImageCount = bufferCount;
	info.imageFormat = m_format;
	info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	info.imageExtent = { width, height };
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = m_allowTearing ? VK_PRESENT_MODE_IMMEDIATE_KHR : VK_PRESENT_MODE_FIFO_KHR;
	info.clipped = VK_TRUE;
	info.oldSwapchain = old;

	XUSG_VK_CHECK(vkCreateSwapchainKHR(m_device, &info, nullptr, &m_swapChain), cerr, false);
	if (old) vkDestroySwapchainKHR(m_device, old, nullptr);

	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
	m_images.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_images.data());
	m_acquired = false;
	if (!m_acquireSemaphore)
	{
		VkSemaphoreCreateInfo semaphore = {};
		semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		XUSG_VK_CHECK(vkCreateSemaphore(m_device, &semaphore, nullptr, &m_acquireSemaphore), cerr, false);
	}
	while (m_renderSemaphores.size() < m_images.size())
	{
		VkSemaphore semaphore = VK_NULL_HANDLE;
		VkSemaphoreCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		XUSG_VK_CHECK(vkCreateSemaphore(m_device, &info, nullptr, &semaphore), cerr, false);
		m_renderSemaphores.push_back(semaphore);
	}

	return acquire();
}

bool SwapChain_VK::acquire() const
{
	if (m_acquired) return true;

	uint32_t index = 0;
	const auto result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_acquireSemaphore, VK_NULL_HANDLE, &index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) return false;
	if (result < 0)
	{
		cerr << "VkResult " << result << endl;
		assert(result >= 0);

		return false;
	}
	m_imageIndex = index;
	m_acquired = true;
	SetQueueAcquireWait(m_queue, m_acquireSemaphore);

	return true;
}

bool SwapChain_VK::Present(uint8_t syncInterval, PresentFlag flags)
{
	(void)syncInterval;
	(void)flags;
	if (!m_acquired && !acquire()) return false;

	VkSemaphore acquireSemaphore = VK_NULL_HANDLE;
	const auto waitAcquire = TakeQueueAcquireWait(m_queue, acquireSemaphore);
	VkSemaphoreSubmitInfo wait = {};
	wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait.semaphore = acquireSemaphore;
	wait.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	VkSemaphoreSubmitInfo signal = {};
	signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signal.semaphore = m_renderSemaphores[m_imageIndex];
	signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	VkSubmitInfo2 submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit.waitSemaphoreInfoCount = waitAcquire ? 1 : 0;
	submit.pWaitSemaphoreInfos = waitAcquire ? &wait : nullptr;
	submit.signalSemaphoreInfoCount = 1;
	submit.pSignalSemaphoreInfos = &signal;
	XUSG_VK_CHECK(vkQueueSubmit2(m_queue, 1, &submit, VK_NULL_HANDLE), cerr, false);

	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &m_renderSemaphores[m_imageIndex];
	info.swapchainCount = 1;
	info.pSwapchains = &m_swapChain;
	info.pImageIndices = &m_imageIndex;
	const auto result = vkQueuePresentKHR(m_queue, &info);
	m_acquired = false;
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return true;
	XUSG_VK_CHECK(result, cerr, false);

	return acquire();
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
	assert(buffer < m_images.size());
	const auto resource = dynamic_cast<Resource_VK*>(pResource);
	assert(resource);
	resource->WrapImage(m_device, m_images[buffer], m_format, m_width, m_height);

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
	if (!m_acquired) acquire();

	return static_cast<uint8_t>(m_imageIndex);
}

void SwapChain_VK::Create(void* pHandle)
{
	m_swapChain = static_cast<VkSwapchainKHR>(pHandle);
}

void* SwapChain_VK::GetHandle() const
{
	return m_swapChain;
}

VkImage SwapChain_VK::GetImage(uint8_t buffer) const
{
	return m_images[buffer];
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
	if (pPipeline->Handle) vkDestroyPipeline(device, pPipeline->Handle, nullptr);
	delete pPipeline;
}
