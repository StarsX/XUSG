//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

//#include "Core/XUSG_DX12.h"
#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Ultimate/XUSGUltimate_DX12.h"
#include "XUSG-EZ_DX12.h"
#include "XUSGUltimate-EZ_DX12.h"

using namespace std;
using namespace XUSG::Ultimate;

EZ::CommandList_DX12::CommandList_DX12() :
	XUSG::CommandList_DX12(),
	XUSG::EZ::CommandList_DX12(),
	m_meshShaderPipelineCache(nullptr),
	m_isMSStateDirty(false),
	m_meshShaderState(nullptr)
{
}

EZ::CommandList_DX12::~CommandList_DX12()
{
}

bool EZ::CommandList_DX12::Create(Ultimate::CommandList* pCommandList, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces)
{
	return true;
}

bool EZ::CommandList_DX12::Create(const Device* pDevice, void* pHandle, uint32_t samplerPoolSize, uint32_t cbvSrvUavPoolSize,
	uint32_t maxSamplers, const uint32_t* pMaxCbvsEachSpace, const uint32_t* pMaxSrvsEachSpace, const uint32_t* pMaxUavsEachSpace,
	uint32_t maxCbvSpaces, uint32_t maxSrvSpaces, uint32_t maxUavSpaces, const wchar_t* name)
{
	m_pDevice = pDevice;
	Ultimate::CommandList_DX12::Create(pHandle, name);

	return Create(this, samplerPoolSize, cbvSrvUavPoolSize, maxSamplers,
		pMaxCbvsEachSpace, pMaxSrvsEachSpace, pMaxUavsEachSpace,
		maxCbvSpaces, maxSrvSpaces, maxUavSpaces);
}

void EZ::CommandList_DX12::ResolveSubresourceRegion(const Resource* pDstResource,
	uint32_t dstSubresource, uint32_t dstX, uint32_t dstY, const Resource* pSrcResource,
	uint32_t srcSubresource, const RectRange& srcRect, Format format, ResolveMode resolveMode)
{
}

void EZ::CommandList_DX12::RSSetShadingRateImage(const Resource* pShadingRateImage)
{
}

void EZ::CommandList_DX12::SetPipelineState(const Pipeline& pipelineState)
{
	XUSG::CommandList_DX12::SetPipelineState(pipelineState);
	m_isGraphicsDirty = false;
	m_isComputeDirty = false;
	m_isMSStateDirty = false;
}

void EZ::CommandList_DX12::MSSetBlendState(MeshShader::BlendPreset preset, uint8_t numColorRTs, uint32_t sampleMask)
{
}

void EZ::CommandList_DX12::MSSetSample(uint8_t count, uint8_t quality)
{
}

void EZ::CommandList_DX12::MSSetRasterizerState(MeshShader::RasterizerPreset preset)
{
}

void EZ::CommandList_DX12::MSSetDepthStencilState(MeshShader::DepthStencilPreset preset)
{
}

void EZ::CommandList_DX12::MSSetShader(Shader::Stage stage, const Blob& shader)
{
}

void EZ::CommandList_DX12::MSSetNodeMask(uint32_t nodeMask)
{
}

void EZ::CommandList_DX12::DispatchMesh(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
}
