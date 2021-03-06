//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGGraphicsState_DX12.h"
#include "XUSGUltimate_DX12.h"
#include "XUSGMeshShaderState_DX12.h"

using namespace std;
using namespace XUSG;

Ultimate::CommandList::uptr Ultimate::CommandList::MakeUnique(API api)
{
	return make_unique<CommandList_DX12>();
}

Ultimate::CommandList::sptr Ultimate::CommandList::MakeShared(API api)
{
	return make_shared<CommandList_DX12>();
}

Ultimate::CommandList::uptr Ultimate::CommandList::MakeUnique(XUSG::CommandList& commandList, API api)
{
	return make_unique<CommandList_DX12>(commandList);
}

Ultimate::CommandList::sptr Ultimate::CommandList::MakeShared(XUSG::CommandList& commandList, API api)
{
	return make_shared<CommandList_DX12>(commandList);
}

Ultimate::SamplerFeedBack::uptr Ultimate::SamplerFeedBack::MakeUnique(API api)
{
	return make_unique<SamplerFeedBack_DX12>();
}

Ultimate::SamplerFeedBack::sptr Ultimate::SamplerFeedBack::MakeShared(API api)
{
	return make_shared<SamplerFeedBack_DX12>();
}

MeshShader::State::uptr MeshShader::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

MeshShader::State::sptr MeshShader::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

MeshShader::PipelineCache::uptr MeshShader::PipelineCache::MakeUnique(API api)
{
	return make_unique<PipelineCache_DX12>();
}

MeshShader::PipelineCache::sptr MeshShader::PipelineCache::MakeShared(API api)
{
	return make_shared<PipelineCache_DX12>();
}

MeshShader::PipelineCache::uptr MeshShader::PipelineCache::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineCache_DX12>(pDevice);
}

MeshShader::PipelineCache::sptr MeshShader::PipelineCache::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineCache_DX12>(pDevice);
}
