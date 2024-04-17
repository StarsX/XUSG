//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGResource_DX12.h"
#include "Core/XUSGGraphicsState_DX12.h"
#include "XUSGUltimate_DX12.h"
#include "XUSGPipelineState_DX12.h"
#include "XUSGWorkGraph_DX12.h"
#include "XUSGGenericState_DX12.h"

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

Ultimate::State::uptr Ultimate::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

Ultimate::State::sptr Ultimate::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

Ultimate::PipelineLib::uptr Ultimate::PipelineLib::MakeUnique(API api)
{
	return make_unique<PipelineLib_DX12>();
}

Ultimate::PipelineLib::sptr Ultimate::PipelineLib::MakeShared(API api)
{
	return make_shared<PipelineLib_DX12>();
}

Ultimate::PipelineLib::uptr Ultimate::PipelineLib::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineLib_DX12>(pDevice);
}

Ultimate::PipelineLib::sptr Ultimate::PipelineLib::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineLib_DX12>(pDevice);
}

Ultimate::SamplerFeedBack::uptr Ultimate::SamplerFeedBack::MakeUnique(API api)
{
	return make_unique<SamplerFeedBack_DX12>();
}

Ultimate::SamplerFeedBack::sptr Ultimate::SamplerFeedBack::MakeShared(API api)
{
	return make_shared<SamplerFeedBack_DX12>();
}

ProgramIdentifier Ultimate::GetProgramIdentifier(const Pipeline& stateObject, const wchar_t* programName)
{
	return GetProgramIdentifierFromDX12(stateObject, programName);
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

Generic::State::uptr Generic::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

Generic::State::sptr Generic::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

Generic::PipelineLib::uptr Generic::PipelineLib::MakeUnique(API api)
{
	return make_unique<PipelineLib_DX12>();
}

Generic::PipelineLib::sptr Generic::PipelineLib::MakeShared(API api)
{
	return make_shared<PipelineLib_DX12>();
}

Generic::PipelineLib::uptr Generic::PipelineLib::MakeUnique(const Device* pDevice, API api)
{
	return make_unique<PipelineLib_DX12>(pDevice);
}

Generic::PipelineLib::sptr Generic::PipelineLib::MakeShared(const Device* pDevice, API api)
{
	return make_shared<PipelineLib_DX12>(pDevice);
}
