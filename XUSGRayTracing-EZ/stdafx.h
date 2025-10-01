//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently.

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#define XUSG_INTERFACE __declspec(dllexport)

#include <windows.h>

#include "d3dx12.h"
#include "d3d12compatibility.h"
#include <dxgi1_5.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

// C RunTime Header Files
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#if _HAS_CXX17
#include <winrt/base.h>
#else
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#endif
#include <functional>
#include <wrl.h>
#include <shellapi.h>

#include "D3D12RaytracingFallback.h"

#if defined(DEBUG) | defined(_DEBUG)
#ifndef DBG_NEW
#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DBG_NEW
#endif
#endif  // _DEBUG
