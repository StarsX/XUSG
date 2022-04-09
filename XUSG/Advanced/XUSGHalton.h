//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef _INDEPENDENT_HALTON_
namespace XUSG
{
	XUSG_INTERFACE float Halton(uint32_t i, uint32_t b);
	XUSG_INTERFACE DirectX::XMFLOAT2 Halton(uint32_t i);
	XUSG_INTERFACE const DirectX::XMFLOAT2& IncrementalHalton();
}
#else
#include "XUSGAdvanced.h"
#endif
