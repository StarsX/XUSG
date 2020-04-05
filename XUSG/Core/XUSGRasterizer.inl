//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

using namespace std;

namespace XUSG
{
	namespace Graphics
	{
		Rasterizer RasterizerDefault()
		{
			return make_shared<D3D12_RASTERIZER_DESC>(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
		}

		Rasterizer CullBack()
		{
			return RasterizerDefault();
		}

		Rasterizer CullNone()
		{
			const auto rasterizer = RasterizerDefault();
			rasterizer->CullMode = D3D12_CULL_MODE_NONE;

			return rasterizer;
		}

		Rasterizer CullFront()
		{
			const auto rasterizer = RasterizerDefault();
			rasterizer->CullMode = D3D12_CULL_MODE_FRONT;

			return rasterizer;
		}

		Rasterizer CullWireframe()
		{
			const auto rasterizer = RasterizerDefault();
			rasterizer->FillMode = D3D12_FILL_MODE_WIREFRAME;

			return rasterizer;
		}
	}
}
