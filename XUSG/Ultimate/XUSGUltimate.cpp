//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "Core/XUSGGraphicsState_DX12.h"
#include "XUSGUltimate_DX12.h"
#include "XUSGMeshletState_DX12.h"

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

Meshlet::State::uptr Meshlet::State::MakeUnique(API api)
{
	return make_unique<State_DX12>();
}

Meshlet::State::sptr Meshlet::State::MakeShared(API api)
{
	return make_shared<State_DX12>();
}

Meshlet::PipelineCache::uptr Meshlet::PipelineCache::MakeUnique(API api)
{
	return make_unique<PipelineCache_DX12>();
}

Meshlet::PipelineCache::sptr Meshlet::PipelineCache::MakeShared(API api)
{
	return make_shared<PipelineCache_DX12>();
}

Meshlet::PipelineCache::uptr Meshlet::PipelineCache::MakeUnique(const Device& device, API api)
{
	return make_unique<PipelineCache_DX12>(device);
}

Meshlet::PipelineCache::sptr Meshlet::PipelineCache::MakeShared(const Device& device, API api)
{
	return make_shared<PipelineCache_DX12>(device);
}
