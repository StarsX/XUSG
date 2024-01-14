//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DXFrameworkHelper.h"
#include "Core/XUSG.h"

#define _XUSG_OCL_11_IMPL_
#include "XUSGOpenCL11.h"

using namespace std;
using namespace XUSG;

const std::map<cl_external_memory_handle_type_khr, const char*> g_handleTypeNames =
{
	{ 0,												"CL_EXTERNAL_MEMORY_HANDLE_NULL" },
	{ CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,			"CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR" },
	{ CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR,		"CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR" },
	{ CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KMT_KHR,	"CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32KMT_KHR" },
	{ CL_EXTERNAL_MEMORY_HANDLE_D3D11_TEXTURE_KHR,		"CL_EXTERNAL_MEMORY_HANDLE_D3D11_TEXTURE_KHR" },
	{ CL_EXTERNAL_MEMORY_HANDLE_D3D11_TEXTURE_KMT_KHR,	"CL_EXTERNAL_MEMORY_HANDLE_D3D11_TEXTURE_KMT_KHR" },
	{ CL_EXTERNAL_MEMORY_HANDLE_D3D12_HEAP_KHR,			"CL_EXTERNAL_MEMORY_HANDLE_D3D12_HEAP_KHR" },
	{ CL_EXTERNAL_MEMORY_HANDLE_D3D12_RESOURCE_KHR,		"CL_EXTERNAL_MEMORY_HANDLE_D3D12_RESOURCE_KHR" },
	{ CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR,			"CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR" }
};

OclContext::~OclContext()
{
	Destroy();
}

cl_int OclContext::Init(const ID3D11Device* pd3dDevice)
{
	enum CL_DX11_EXT
	{
		CL_DX11_EXT_NONE,
		CL_DX11_EXT_KHR,
		CL_DX11_EXT_NV
	} clDX11Ext;

	cl_uint numPlatforms = 0;

	// [1] get the platform
	auto status = clGetPlatformIDs(0, nullptr, &numPlatforms);
	XUSG_C_RETURN(CheckStatus(status, "clGetPlatformIDs error"), status);

	vector<cl_platform_id> platforms(numPlatforms);
	status = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
	XUSG_C_RETURN(CheckStatus(status, "clGetPlatformIDs error"), status);

	// [2] get device ids for the platform i have obtained
	for (const auto& platform : platforms)
	{
		size_t platformNameSize = 0;
		status = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, nullptr, &platformNameSize);
		XUSG_C_RETURN(CheckStatus(status, "clGetPlatformInfo error"), status);

		string platformName(platformNameSize, '\0');
		status = clGetPlatformInfo(platform, CL_PLATFORM_NAME, platformName.size(), &platformName[0], nullptr);
		XUSG_C_RETURN(CheckStatus(status, "clGetPlatformInfo error"), status);

		size_t extensionsSize = 0;
		status = clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, 0, nullptr, &extensionsSize);
		XUSG_C_RETURN(CheckStatus(status, "clGetPlatformInfo error"), status);

		string extensions(extensionsSize, '\0');
		status = clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, extensions.size(), &extensions[0], nullptr);
		XUSG_C_RETURN(CheckStatus(status, "clGetPlatformInfo error"), status);
		cout << "Platform " << platformName << "extensions supported: " << extensions << endl;
		cout << endl;

		const char* extKHR = strstr(extensions.c_str(), "cl_khr_d3d11_sharing");
		const char* extNV = strstr(extensions.c_str(), "cl_nv_d3d11_sharing");
		if (extKHR) clDX11Ext = CL_DX11_EXT_KHR;
		else if (extNV) clDX11Ext = CL_DX11_EXT_NV;
		else
		{
			cerr << "Platform " << platformName << " does support any cl_*_d3d11_sharing" << endl;

			return CL_INVALID_PLATFORM;
		}

		assert(clDX11Ext == CL_DX11_EXT_KHR || clDX11Ext == CL_DX11_EXT_NV);
		const auto clGetDeviceIDsFromD3D11 = (clGetDeviceIDsFromD3D11KHR_fn)clGetExtensionFunctionAddressForPlatform(
			platform, clDX11Ext == CL_DX11_EXT_KHR ? "clGetDeviceIDsFromD3D11KHR" : "clGetDeviceIDsFromD3D11NV");
		XUSG_N_RETURN(clGetDeviceIDsFromD3D11, CL_INVALID_PLATFORM);

		clCreateFromD3D11Buffer = (clCreateFromD3D11BufferKHR_fn)clGetExtensionFunctionAddressForPlatform(platform,
			clDX11Ext == CL_DX11_EXT_KHR ? "clCreateFromD3D11BufferKHR" : "clCreateFromD3D11BufferNV");
		if (!clCreateFromD3D11Buffer) cout << "Warning: clCreateFromD3D11Buffer is not surpported" << endl;

		clCreateFromD3D11Texture2D = (clCreateFromD3D11Texture2DKHR_fn)clGetExtensionFunctionAddressForPlatform(platform,
			clDX11Ext == CL_DX11_EXT_KHR ? "clCreateFromD3D11Texture2DKHR" : "clCreateFromD3D11Texture2DNV");
		if (!clCreateFromD3D11Texture2D) cout << "Warning: clCreateFromD3D11Texture2D is not surpported" << endl;

		clCreateFromD3D11Texture3D = (clCreateFromD3D11Texture3DKHR_fn)clGetExtensionFunctionAddressForPlatform(platform,
			clDX11Ext == CL_DX11_EXT_KHR ? "clCreateFromD3D11Texture#DKHR" : "clCreateFromD3D11Texture3DNV");
		if (!clCreateFromD3D11Texture3D) cout << "Warning: clCreateFromD3D11Texture3D is not surpported" << endl;

		clEnqueueAcquireD3D11Objects = (clEnqueueAcquireD3D11ObjectsKHR_fn)clGetExtensionFunctionAddressForPlatform(platform,
			clDX11Ext == CL_DX11_EXT_KHR ? "clEnqueueAcquireD3D11ObjectsKHR" : "clEnqueueAcquireD3D11ObjectsNV");
		if (!clEnqueueAcquireD3D11Objects) cout << "Warning: clEnqueueAcquireD3D11Objects is not surpported" << endl;

		clEnqueueReleaseD3D11Objects = (clEnqueueReleaseD3D11ObjectsKHR_fn)clGetExtensionFunctionAddressForPlatform(platform,
			clDX11Ext == CL_DX11_EXT_KHR ? "clEnqueueReleaseD3D11ObjectsKHR" : "clEnqueueReleaseD3D11ObjectsNV");
		if (!clEnqueueReleaseD3D11Objects) cout << "Warning: clEnqueueReleaseD3D11Objects is not surpported" << endl;

		if (strstr(extensions.c_str(), "cl_khr_external_memory"))
		{
			size_t handleTypesSize = 0;
			status = clGetPlatformInfo(platform, CL_PLATFORM_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR, 0, nullptr, &handleTypesSize);
			XUSG_C_RETURN(CheckStatus(status, "clGetPlatformInfo error"), status);

			vector<cl_external_memory_handle_type_khr> handleTypes(handleTypesSize);
			status = clGetPlatformInfo(platform, CL_PLATFORM_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR, handleTypes.size(), &handleTypes[0], nullptr);
			XUSG_C_RETURN(CheckStatus(status, "clGetPlatformInfo error"), status);
			cout << "Platform " << platformName << "cl_external_memory_handle_type_khr supported: ";
			for (size_t i = 0; i < handleTypesSize; ++i) cout << g_handleTypeNames.find(handleTypes[i])->second << " ";
			cout << endl;
			cout << endl;

			clEnqueueAcquireExternalMemObjects = (clEnqueueAcquireD3D11ObjectsKHR_fn)clGetExtensionFunctionAddressForPlatform(platform, "clEnqueueAcquireExternalMemObjectsKHR");
			if (!clEnqueueAcquireExternalMemObjects) cout << "Warning: clEnqueueAcquireExternalMemObjects is not surpported" << endl;

			clEnqueueReleaseExternalMemObjects = (clEnqueueReleaseD3D11ObjectsKHR_fn)clGetExtensionFunctionAddressForPlatform(platform, "clEnqueueReleaseExternalMemObjectsKHR");
			if (!clEnqueueReleaseExternalMemObjects) cout << "Warning: clEnqueueReleaseExternalMemObjects is not surpported" << endl;

			//clCreateImageFromExternalMemory = (clCreateImageFromExternalMemoryKHR_fn)clGetExtensionFunctionAddressForPlatform(platform, "clCreateImageFromExternalMemoryKHR");
			//if (!clCreateImageFromExternalMemory) cout << "Warning: clCreateImageFromExternalMemory is not surpported" << endl;
		}

		cl_uint numDevices = 0;
		status = clGetDeviceIDsFromD3D11(platform, CL_D3D11_DEVICE_KHR, (void*)pd3dDevice, CL_PREFERRED_DEVICES_FOR_D3D11_KHR, 0, nullptr, &numDevices);
		XUSG_C_RETURN(CheckStatus(status, "Failed on clGetDeviceIDsFromD3D11"), status);

		if (numDevices > 0)
		{
			m_devices.resize(numDevices);
			status = clGetDeviceIDsFromD3D11(platform, CL_D3D11_DEVICE_KHR, (void*)pd3dDevice, CL_PREFERRED_DEVICES_FOR_D3D11_KHR, numDevices, m_devices.data(), nullptr);
			XUSG_C_RETURN(CheckStatus(status, "Failed on clGetDeviceIDsFromD3D11"), status);

			// create an OCL context from the device we are using as our DX11 rendering device
			cl_context_properties cps[] =
			{
				CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
				CL_CONTEXT_D3D11_DEVICE_KHR, (cl_context_properties)pd3dDevice,
				//CL_CONTEXT_INTEROP_USER_SYNC, CL_FALSE,
				0
			};
			m_context = clCreateContext(cps, numDevices, m_devices.data(), nullptr, nullptr, &status);
			XUSG_C_RETURN(CheckStatus(status, "clCreateContext error"), status);

			for (const auto& device : m_devices)
			{
				size_t deviceNameSize = 0;
				status = clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &deviceNameSize);
				XUSG_C_RETURN(CheckStatus(status, "clGetDeviceInfo error"), status);

				string deviceName(deviceNameSize, '\0');
				status = clGetDeviceInfo(device, CL_DEVICE_NAME, deviceName.size(), &deviceName[0], nullptr);
				XUSG_C_RETURN(CheckStatus(status, "clGetDeviceInfo error"), status);

				size_t extensionsSize = 0;
				status = clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, 0, nullptr, &extensionsSize);
				XUSG_C_RETURN(CheckStatus(status, "clGetDeviceInfo error"), status);

				string extensions(extensionsSize, '\0');
				status = clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, extensions.size(), &extensions[0], nullptr);
				XUSG_C_RETURN(CheckStatus(status, "clGetDeviceInfo error"), status);
				cout << "Device " << deviceName << "extensions supported: " << extensions << endl;
				cout << endl;

				if (strstr(extensions.c_str(), "cl_khr_external_memory"))
				{
					size_t handleTypesSize = 0;
					status = clGetDeviceInfo(device, CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR, 0, nullptr, &handleTypesSize);
					XUSG_C_RETURN(CheckStatus(status, "clGetDeviceInfo error"), status);

					vector<cl_external_memory_handle_type_khr> handleTypes(handleTypesSize);
					status = clGetDeviceInfo(device, CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR, handleTypes.size(), &handleTypes[0], nullptr);
					XUSG_C_RETURN(CheckStatus(status, "clGetDeviceInfo error"), status);
					cout << "Device " << deviceName << "cl_external_memory_handle_type_khr supported: ";
					for (size_t i = 0; i < handleTypesSize; ++i) cout << g_handleTypeNames.find(handleTypes[i])->second << " ";
					cout << endl;
					cout << endl;
				}
			}

			// create an openCL commandqueue
			// the queue and move on, the sample is about sharing, not about robust device call/response/create patterns
			m_queue = clCreateCommandQueueWithProperties(m_context, m_devices[0], 0, &status);
			//m_queue = clCreateCommandQueue(clContext, m_devices[0], 0, &status);
			XUSG_C_RETURN(CheckStatus(status, "clCreateCommandQueue error"), status);

			m_platform = platform;
			break;
		}
	}

	return CL_SUCCESS;
}

cl_int OclContext::CheckExternalMemoryHandleType(cl_external_memory_handle_type_khr requiredHandleType, cl_uint deviceIndex) const
{
	static vector<cl_external_memory_handle_type_khr> handleTypes(0);
	cl_int status = CL_SUCCESS;

	if (handleTypes.empty())
	{
		size_t handleTypesSize = 0;
		const auto& device = m_devices[deviceIndex];
		status = clGetDeviceInfo(device, CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR, 0, nullptr, &handleTypesSize);

		if (CL_SUCCESS != status) {
			cout << "Unable to query CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR" << endl;
			return status;
		}

		handleTypes.resize(handleTypesSize);

		status = clGetDeviceInfo(device, CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR, handleTypesSize, &handleTypes[0], nullptr);

		if (CL_SUCCESS != status) {
			cout << "Unable to query CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR" << endl;
			return status;
		}
	}

	const auto handleTypesSize = static_cast<uint8_t>(handleTypes.size());
	for (uint8_t i = 0; i < handleTypesSize; ++i)
	{
		const auto a = handleTypes[i];
		if (requiredHandleType == handleTypes[i]) return CL_SUCCESS;
	}
	cout << "cl_khr_external_memory extension is missing support for " << g_handleTypeNames.find(requiredHandleType)->second << endl;

	return CL_INVALID_VALUE;
}

cl_int OclContext::Destroy()
{
	if (m_queue)
	{
		const auto status = clReleaseCommandQueue(m_queue);
		XUSG_C_RETURN(CheckStatus(status, "clReleaseCommandQueue error"), status);
		m_queue = nullptr;
	}

	if (m_context)
	{
		const auto status = clReleaseContext(m_context);
		XUSG_C_RETURN(CheckStatus(status, "clReleaseContext error"), status);
		m_context = nullptr;
	}

	return CL_SUCCESS;
}

cl_platform_id OclContext::GetPlatform() const
{
	return m_platform;
}

cl_context OclContext::GetContext() const
{
	return m_context;
}

cl_device_id OclContext::GetDevice(cl_uint index) const
{
	return m_devices.at(index);
}

cl_command_queue OclContext::GetQueue() const
{
	return m_queue;
}

//if an error occurs we exit
//it would be better to cleanup state then exit, for sake of simplicity going to omit the cleanup
cl_int OclContext::CheckStatus(cl_int status, const char* errorMsg)
{
	if (status != CL_SUCCESS)
	{
		if (errorMsg == nullptr) cerr << "Error" << endl;
		else cerr << "Error: " << errorMsg << endl;
	}

	return status;
}

OclContext11::~OclContext11()
{
}

bool OclContext11::Init(IDXGIAdapter* pAdapter)
{
	// Create DX11 device
	uint32_t d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;
#if defined(_DEBUG)
	//d3d11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	com_ptr<ID3D11Device> d3d11Device;
	XUSG_C_RETURN(FAILED(D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
		d3d11DeviceFlags, nullptr, 0, D3D11_SDK_VERSION, d3d11Device.put(), nullptr, nullptr)),
		false);

	// Create OpenCL context from DX11 device
	XUSG_C_RETURN(OclContext::Init(d3d11Device.get()) != CL_SUCCESS, false);
	XUSG_C_RETURN(FAILED(d3d11Device->QueryInterface<ID3D11Device1>(m_device11.put())), false);

	return true;
}

com_ptr<ID3D11Buffer> OclContext11::CreateStructuredBuffer11(uint32_t numElements, uint32_t stride,
	uint32_t bindFlags11, cl_mem* pBufferCL, cl_mem_flags clMemFlag) const
{
	com_ptr<ID3D11Buffer> buffer11 = nullptr;

	if (m_device11)
	{
		const CD3D11_BUFFER_DESC desc(stride * numElements, bindFlags11, D3D11_USAGE_DEFAULT, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, stride);

		// Create DX11 resource
		XUSG_H_RETURN(m_device11->CreateBuffer(&desc, nullptr, buffer11.put()),
			cerr, ("Failed to create DX11 buffer, " + HrToString(hr)).c_str(), nullptr);

		// Wrap OpenCL resource
		if (pBufferCL)
		{
			cl_int status = CL_SUCCESS;
			*pBufferCL = clCreateFromD3D11Buffer(m_context, clMemFlag, buffer11.get(), &status);
			XUSG_C_RETURN(CheckStatus(status, "clCreateFromD3D11Buffer error") != CL_SUCCESS, nullptr);
		}
	}
	else cerr << "Invalid DX11 device." << endl;

	return buffer11;
}

com_ptr<ID3D11Texture2D>OclContext11::CreateTexture2D11(DXGI_FORMAT format, uint32_t width, uint32_t height,
	uint32_t arraySize, uint8_t mipLevels, uint32_t bindFlags11, cl_mem* pTextureCL, cl_mem_flags clMemFlag) const
{
	com_ptr<ID3D11Texture2D> texture11 = nullptr;

	if (m_device11)
	{
		const CD3D11_TEXTURE2D_DESC desc(format, width, height, arraySize, mipLevels, bindFlags11);

		// Create DX11 resource
		XUSG_H_RETURN(m_device11->CreateTexture2D(&desc, nullptr, texture11.put()),
			cerr, ("Failed to create DX11 texture, " + HrToString(hr)).c_str(), nullptr);

		// Wrap OpenCL resource
		if (pTextureCL)
		{
			cl_int status = CL_SUCCESS;
			*pTextureCL = clCreateFromD3D11Texture2D(m_context, clMemFlag, texture11.get(), 0, &status);
			XUSG_C_RETURN(CheckStatus(status, "clCreateFromD3D11Texture2D error") != CL_SUCCESS, nullptr);
		}
	}
	else cerr << "Invalid DX11 device." << endl;

	return texture11;
}

com_ptr<ID3D11Device1> OclContext11::GetDevice11() const
{
	return m_device11;
}

com_ptr<ID3D11Texture2D> OclContext11::GetTexture11(const Device* pDevice, const Texture* pTexture, const wchar_t* name) const
{
	assert(pDevice);
	const auto pDevice12 = static_cast<ID3D12Device*>(pDevice->GetHandle());
	assert(pDevice12);

	com_ptr<ID3D11Texture2D> texture11 = nullptr;

	if (m_device11)
	{
		// Create resource handle on DX12
		HANDLE hResource;
		const auto pResource12 = static_cast<ID3D12Resource*>(pTexture->GetHandle());
		XUSG_H_RETURN(pDevice12->CreateSharedHandle(pResource12, nullptr, GENERIC_ALL,
			name, &hResource), cerr, ("Failed to share DX12 resource, " + HrToString(hr)).c_str(), nullptr);

		// Open resource handle on DX11
		XUSG_H_RETURN(m_device11->OpenSharedResource1(hResource, IID_PPV_ARGS(&texture11)),
			cerr, ("Failed to open shared resource on DX11, " + HrToString(hr)).c_str(), nullptr);
	}
	else cerr << "Invalid DX11 device." << endl;

	return texture11;
}
