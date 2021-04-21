//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "XUSGRayTracing_DX12.h"

using namespace std;
using namespace XUSG::RayTracing;

Device_DX12::Device_DX12()
{
}

Device_DX12::~Device_DX12()
{
}

#if ENABLE_DXR_FALLBACK
bool Device_DX12::CreateInterface(uint8_t flags)
{
	const auto hr = D3D12CreateRaytracingFallbackDevice(m_device.get(), flags, 0, IID_PPV_ARGS(&m_deviceRT));
	if (FAILED(hr))
	{
		OutputDebugString(L"Couldn't get DirectX Raytracing Fallback interface for the device.\n");

		return false;
	}

	return true;
}
#else
bool Device_DX12::CreateInterface()
{
	const auto hr = m_device->QueryInterface(IID_PPV_ARGS(&m_deviceRT));
	if (FAILED(hr))
	{
		OutputDebugString(L"Couldn't get DirectX Raytracing interface for the device.\n");

		return false;
	}

	return true;
}
#endif

void* Device_DX12::GetRTHandle() const
{
	return m_deviceRT.get();
}
