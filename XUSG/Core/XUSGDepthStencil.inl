//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

using namespace std;

namespace XUSG
{
	namespace Graphics
	{
		DepthStencil DepthStencilDefault()
		{
			return make_shared<D3D12_DEPTH_STENCIL_DESC>(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
		}

		DepthStencil DepthStencilNone()
		{
			const auto depthStencil = DepthStencilDefault();
			depthStencil->DepthEnable = FALSE;
			depthStencil->DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

			return depthStencil;
		}

		DepthStencil DepthRead()
		{
			const auto depthStencil = DepthStencilDefault();
			depthStencil->DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

			return depthStencil;
		}

		DepthStencil DepthReadLessEqual()
		{
			const auto depthStencil = DepthStencilDefault();
			depthStencil->DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
			depthStencil->DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

			return depthStencil;
		}

		DepthStencil DepthReadEqual()
		{
			const auto depthStencil = DepthStencilDefault();
			depthStencil->DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
			depthStencil->DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

			return depthStencil;
		}
	}
}
