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
	};

	void RetainVKPipeline(Pipeline_VK* pPipeline);
	void ReleaseVKPipeline(VkDevice device, Pipeline_VK* pPipeline);

	struct QueueNative
	{
		VkQueue Queue;
		VkDevice Device;
		VkPhysicalDevice PhysicalDevice;
		VkInstance Instance;
		uint32_t Family;
		uint32_t ApiVersion;
	};

	void RegisterQueue(const QueueNative& queue);
	void UnregisterQueue(VkQueue queue);
	bool FindQueue(VkQueue queue, QueueNative& native);
	void SetQueueAcquireWait(VkQueue queue, VkSemaphore semaphore);
	bool TakeQueueAcquireWait(VkQueue queue, VkSemaphore& semaphore);
	void ClearQueueAcquireWait(VkQueue queue);

	struct BufferNative
	{
		VkBuffer Buffer;
		VkDeviceAddress Address;
		VkDeviceSize Size;
	};

	void RegisterBuffer(const BufferNative& buffer);
	void UnregisterBuffer(VkBuffer buffer);
	bool FindBuffer(VkDeviceAddress address, BufferNative& native);

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
			const CommandAllocator* pCommandAllocator, const Pipeline& pipeline,
			const wchar_t* name = nullptr);
		bool GetFence(Fence* pFence, uint64_t initialValue, FenceFlag flags,
			const wchar_t* name = nullptr);

		uint32_t Create(void* pAdapter, uint32_t minFeatureLevel, const wchar_t* name = nullptr);
		uint32_t GetDeviceRemovedReason() const;

		void Create(void* pHandle, const wchar_t* name = nullptr);

		void* GetHandle() const;

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
			PFN_vkWriteSamplerDescriptorsEXT WriteSamplerDescriptors;
			PFN_vkWriteResourceDescriptorsEXT WriteResourceDescriptors;
			PFN_vkCmdBeginConditionalRenderingEXT BeginConditionalRendering;
			PFN_vkCmdEndConditionalRenderingEXT EndConditionalRendering;
		};

		const ExtProcs& GetExtProcs() const;

		bool HasDebugUtils() const;
		bool HasCustomBorderColor() const;
		bool HasMeshShader() const;
		bool HasFragmentShadingRate() const;
		bool HasConditionalRendering() const;
		bool HasConservativeRasterization() const;

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
		bool				m_meshShader;
		bool				m_fragmentShadingRate;
		bool				m_conditionalRendering;
		bool				m_conservativeRaster;
		bool				m_ownsInstance;

		VkPhysicalDeviceDescriptorHeapPropertiesEXT				m_heapProps;
		VkPhysicalDeviceDeviceGeneratedCommandsPropertiesEXT	m_dgcProps;

		PFN_vkCmdBeginRendering			m_beginRendering;
		PFN_vkCmdEndRendering			m_endRendering;
		PFN_vkCmdSetPrimitiveTopology	m_setPrimitiveTopology;
		PFN_vkCmdBindVertexBuffers2		m_bindVertexBuffers2;
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
		mutable std::mutex	m_queueMutex;
	};

	class Fence_VK :
		public virtual Fence
	{
	public:
		Fence_VK();
		virtual ~Fence_VK();

		bool Create(const Device* pDevice, uint64_t initialValue,
			FenceFlag flags, const wchar_t* name = nullptr);
		bool SetEventOnCompletion(uint64_t value, void* hEvent);
		bool Signal(uint64_t value);

		uint64_t GetCompletedValue() const;

		void Create(void* pHandle, const wchar_t* name = nullptr);

		void* GetHandle() const;

		VkSemaphore GetSemaphore() const;

	protected:
		VkDevice		m_device;
		VkSemaphore		m_semaphore;
		const Device*	m_pDevice;
	};

	class CommandLayout_VK :
		public virtual CommandLayout
	{
	public:
		CommandLayout_VK();
		virtual ~CommandLayout_VK();

		bool Create(const Device* pDevice, uint32_t byteStride, uint32_t numArguments,
			const IndirectArgument* pArguments, const PipelineLayout& pipelineLayout = nullptr,
			uint32_t nodeMask = 0, const wchar_t* name = nullptr);

		void Create(void* pHandle, const wchar_t* name = nullptr);

		void* GetHandle() const;

		VkIndirectCommandsLayoutEXT GetLayout() const;
		VkShaderStageFlags GetShaderStages() const;
		uint32_t GetByteStride() const;
		bool IsDispatch() const;

	protected:
		VkDevice						m_device;
		const Device_VK*				m_pDevice;
		VkIndirectCommandsLayoutEXT		m_layout;
		VkShaderStageFlags				m_stages;
		uint32_t						m_stride;
		bool							m_isDispatch;

		std::vector<VkIndirectCommandsLayoutTokenEXT>			m_tokens;
		std::vector<VkIndirectCommandsVertexBufferTokenEXT>		m_vertexTokens;
		std::vector<VkIndirectCommandsIndexBufferTokenEXT>		m_indexTokens;
		std::vector<VkIndirectCommandsPushConstantTokenEXT>		m_pushTokens;
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

		void Create(void* pHandle);

		void* GetHandle() const;

		VkImage GetImage(uint8_t buffer) const;
		VkFormat GetVkFormat() const;
		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

	protected:
		bool createSwapchain(uint8_t bufferCount, uint32_t width, uint32_t height, Format format, VkSwapchainKHR old);
		bool acquire() const;

		VkInstance			m_instance;
		VkPhysicalDevice	m_physicalDevice;
		VkDevice			m_device;
		VkQueue				m_queue;
		uint32_t			m_family;
		VkSurfaceKHR		m_surface;
		VkSwapchainKHR		m_swapChain;

		VkFormat			m_format;
		uint32_t			m_width;
		uint32_t			m_height;
		uint8_t				m_bufferCount;
		mutable uint32_t	m_imageIndex;
		bool				m_allowTearing;
		mutable bool		m_acquired;

		VkSemaphore			m_acquireSemaphore;
		std::vector<VkSemaphore> m_renderSemaphores;

		std::vector<VkImage> m_images;
	};

	void GetVKBlobData(const Blob& blob, const void** ppData, size_t* pSize);
	Blob GetVKPipelineCache(Pipeline pipeline);

	std::string WideToUtf8(const wchar_t* name);
}
