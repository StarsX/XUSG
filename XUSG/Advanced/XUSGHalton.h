//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace XUSG
{
	XUSG_INTERFACE float Halton(uint32_t i, uint32_t b);
	XUSG_INTERFACE DirectX::XMFLOAT2 Halton(uint32_t i);
	XUSG_INTERFACE const DirectX::XMFLOAT2& IncrementalHalton();
}
