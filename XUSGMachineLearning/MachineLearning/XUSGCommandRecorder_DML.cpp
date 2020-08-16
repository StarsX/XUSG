//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSGCommand_DX12.h"
#include "XUSGCommandRecorder_DML.h"

using namespace XUSG;
using namespace XUSG::ML;

CommandRecorder_DML::CommandRecorder_DML()
{
}

CommandRecorder_DML::~CommandRecorder_DML()
{
}

void CommandRecorder_DML::Dispatch(CommandList* pCommandList, const Dispatchable& dispatchable, const BindingTable& bindings) const
{
	m_commandRecorder->RecordDispatch(dynamic_cast<CommandList_DX12*>(pCommandList)->GetGraphicsCommandList().get(),
		dispatchable.get(), bindings.get());
}

com_ptr<IDMLCommandRecorder>& CommandRecorder_DML::GetDMLCommandRecorder()
{
	return m_commandRecorder;
}

//--------------------------------------------------------------------------------------

bool DMLDevice::GetCommandRecorder(CommandRecorder* pCommandRecorder)
{
	return GetCommandRecorder(*pCommandRecorder);
}

bool DMLDevice::GetCommandRecorder(CommandRecorder& commandRecorder)
{
	auto& pDMLCommandRecorder = dynamic_cast<CommandRecorder_DML&>(commandRecorder).GetDMLCommandRecorder();
	V_RETURN(CreateCommandRecorder(IID_PPV_ARGS(&pDMLCommandRecorder)), std::cerr, false);

	return true;
}
