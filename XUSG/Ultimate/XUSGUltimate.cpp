//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGGraphicsState_DX12.h"
#include "XUSGUltimate_DX12.h"
#include "XUSGMeshShaderState_DX12.h"
#include "XUSGWorkGraph_DX12.h"

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

ProgramIdentifier Ultimate::GetProgramIdentifier(const Pipeline& stateObject, const void* program)
{
	return GetProgramIdentifierFromDX12(stateObject, program);
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

MeshShader::PipelineLib::uptr MeshShader::PipelineLib::MakeUnique(API api)
{
	return make_unique<PipelineLib_DX12>();
}

MeshShader::PipelineLib::sptr MeshShader::PipelineLib::MakeShared(API api)
{
	return make_shared<PipelineLib_DX12>();
}

MeshShader::PipelineLib::uptr MeshShader::PipelineLib::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineLib_DX12>(pDevice);
}

MeshShader::PipelineLib::sptr MeshShader::PipelineLib::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineLib_DX12>(pDevice);
}

WorkGraph::State::uptr WorkGraph::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

WorkGraph::State::sptr WorkGraph::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

WorkGraph::PipelineLib::uptr WorkGraph::PipelineLib::MakeUnique(API api)
{
	return make_unique<PipelineLib_DX12>();
}

WorkGraph::PipelineLib::sptr WorkGraph::PipelineLib::MakeShared(API api)
{
	return make_shared<PipelineLib_DX12>();
}

WorkGraph::PipelineLib::uptr WorkGraph::PipelineLib::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineLib_DX12>(pDevice);
}

WorkGraph::PipelineLib::sptr WorkGraph::PipelineLib::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineLib_DX12>(pDevice);
}
