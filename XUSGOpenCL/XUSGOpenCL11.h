//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#define XUSG_H_RETURN(x, o, m, r)	{ const auto hr = x; if (FAILED(hr)) { o << m << std::endl; assert(!m); return r; } }

namespace XUSG
{
	class OclContext
	{
	public:
		virtual ~OclContext();

		cl_int Init(const ID3D11Device* pd3dDevice);
		cl_int CheckExternalMemoryHandleType(cl_external_memory_handle_type_khr requiredHandleType, cl_uint deviceIndex = 0) const;
		cl_int Destroy();

		cl_platform_id GetPlatform() const;
		cl_context GetContext() const;
		cl_device_id GetDevice(cl_uint index = 0) const;
		cl_command_queue GetQueue() const;

		static cl_int CheckStatus(cl_int status, const char* errorMsg);

	protected:
		cl_platform_id m_platform;
		cl_context m_context;
		std::vector<cl_device_id> m_devices;
		cl_command_queue m_queue;
	};

	class OclContext11 : public OclContext
	{
	public:
		virtual ~OclContext11();

		bool Init(IDXGIAdapter* pAdapter);

		com_ptr<ID3D11Buffer> CreateStructuredBuffer11(uint32_t numElements, uint32_t stride,
			uint32_t bindFlags11, cl_mem* pBufferCL = nullptr, cl_mem_flags clMemFlag = CL_MEM_READ_WRITE) const;

		com_ptr<ID3D11Texture2D> CreateTexture2D11(DXGI_FORMAT format, uint32_t width, uint32_t height,
			uint32_t arraySize, uint8_t mipLevels, uint32_t bindFlags11, cl_mem* pTextureCL = nullptr,
			cl_mem_flags clMemFlag = CL_MEM_READ_WRITE) const;

		com_ptr<ID3D11Device1> GetDevice11() const;

		com_ptr<ID3D11Texture2D> GetTexture11(const Device* pDevice, const Texture* pTexture, const wchar_t* name = nullptr) const;

	protected:
		com_ptr<ID3D11Device1> m_device11;
	};

#ifdef _XUSG_OCL_11_IMPL_
#define XUSG_EXTERN
#else
#define XUSG_EXTERN extern
#endif

	XUSG_EXTERN clCreateFromD3D11BufferKHR_fn clCreateFromD3D11Buffer;
	XUSG_EXTERN clCreateFromD3D11Texture2DKHR_fn clCreateFromD3D11Texture2D;
	XUSG_EXTERN clCreateFromD3D11Texture3DKHR_fn clCreateFromD3D11Texture3D;
	XUSG_EXTERN clEnqueueAcquireD3D11ObjectsKHR_fn clEnqueueAcquireD3D11Objects;
	XUSG_EXTERN clEnqueueReleaseD3D11ObjectsKHR_fn clEnqueueReleaseD3D11Objects;
	XUSG_EXTERN clEnqueueAcquireExternalMemObjectsKHR_fn clEnqueueAcquireExternalMemObjects;
	XUSG_EXTERN clEnqueueReleaseExternalMemObjectsKHR_fn clEnqueueReleaseExternalMemObjects;
	//XUSG_EXTERN clCreateImageFromExternalMemoryKHR_fn clCreateImageFromExternalMemory;
}
