//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSG-EZ.h"
#include "Ultimate/XUSGUltimate.h"

namespace XUSG
{
	namespace Ultimate
	{
		namespace EZ
		{
			XUSG_INTERFACE XUSG::EZ::ResourceView GetUAV(SamplerFeedBack* pResource);

			//--------------------------------------------------------------------------------------
			// Ultimate command list
			//--------------------------------------------------------------------------------------
			class XUSG_INTERFACE CommandList :
				public virtual XUSG::EZ::CommandList
			{
			public:
				//CommandList();
				virtual ~CommandList() {}

				using uptr = std::unique_ptr<CommandList>;
				using sptr = std::shared_ptr<CommandList>;

				// By default maxCbvsEachSpace = 14, maxSrvsEachSpace = 32, and maxUavsEachSpace = 16
				virtual bool Create(Ultimate::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1) = 0;
				virtual bool Create(const Device* pDevice, void* pHandle, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					const wchar_t* name = nullptr) = 0;

				virtual void SetSamplePositions(uint8_t numSamplesPerPixel, uint8_t numPixels, SamplePosition* pPositions) const = 0;
				virtual void ResolveSubresourceRegion(Resource* pDstResource, uint32_t dstSubresource,
					uint32_t dstX, uint32_t dstY, Resource* pSrcResource, uint32_t srcSubresource,
					const RectRange& srcRect, Format format, ResolveMode resolveMode) = 0;

				virtual void RSSetShadingRate(ShadingRate baseShadingRate, const ShadingRateCombiner* pCombiners) const = 0;
				virtual void RSSetShadingRateImage(Resource* pShadingRateImage) = 0;

				virtual void SetPipelineState(const Pipeline& pipelineState) = 0;
				virtual void MSSetBlendState(MeshShader::BlendPreset preset, uint8_t numColorRTs = 1, uint32_t sampleMask = UINT_MAX) = 0;
				virtual void MSSetSample(uint8_t count, uint8_t quality = 0) = 0;
				virtual void MSSetRasterizerState(MeshShader::RasterizerPreset preset) = 0;
				virtual void MSSetDepthStencilState(MeshShader::DepthStencilPreset preset) = 0;
				virtual void MSSetShader(Shader::Stage stage, const Blob& shader) = 0;
				virtual void MSSetNodeMask(uint32_t nodeMask) = 0;
				virtual void DispatchMesh(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ) = 0;

				static uptr MakeUnique(API api = API::DIRECTX_12);
				static sptr MakeShared(API api = API::DIRECTX_12);
				static uptr MakeUnique(Ultimate::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					API api = API::DIRECTX_12);
				static sptr MakeShared(Ultimate::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
					uint32_t maxSamplers = 16, const uint32_t* pMaxCbvsEachSpace = nullptr, const uint32_t* pMaxSrvsEachSpace = nullptr,
					const uint32_t* pMaxUavsEachSpace = nullptr, uint32_t maxCbvSpaces = 1, uint32_t maxSrvSpaces = 1, uint32_t maxUavSpaces = 1,
					API api = API::DIRECTX_12);
			};
		}
	}
}
