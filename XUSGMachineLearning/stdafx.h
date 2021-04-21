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

#define DLL_INTERFACE __declspec(dllexport)

#include <windows.h>

#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#include <DirectML.h>					// The DirectML header from the Windows SDK.

// C RunTime Header Files
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#if _HAS_CXX17
#include <winrt/base.h>
#else
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#endif
#include <functional>
#include <wrl.h>
#include <shellapi.h>

#define ML_MANAGED_WEIGHTS	1

#if defined(DEBUG) | defined(_DEBUG)
#ifndef DBG_NEW
#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DBG_NEW
#endif
#endif  // _DEBUG
