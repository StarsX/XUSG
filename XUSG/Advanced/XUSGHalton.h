//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef _INDEPENDENT_HALTON_
namespace XUSG
{
	DLL_INTERFACE float Halton(uint32_t i, uint32_t b);
	DLL_INTERFACE DirectX::XMFLOAT2 Halton(uint32_t i);
	DLL_INTERFACE const DirectX::XMFLOAT2& IncrementalHalton();
}
#else
#include "XUSGAdvanced.h"
#endif
