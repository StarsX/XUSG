//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
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
	m_commandRecorder->RecordDispatch(static_cast<ID3D12CommandList*>(pCommandList->GetHandle()),
		static_cast<IDMLDispatchable*>(dispatchable), static_cast<IDMLBindingTable*>(bindings));
}

com_ptr<IDMLCommandRecorder>& CommandRecorder_DML::GetDMLCommandRecorder()
{
	return m_commandRecorder;
}

//--------------------------------------------------------------------------------------

bool DMLDevice::GetCommandRecorder(CommandRecorder* pCommandRecorder)
{
	auto& pDMLCommandRecorder = dynamic_cast<CommandRecorder_DML*>(pCommandRecorder)->GetDMLCommandRecorder();
	V_RETURN(CreateCommandRecorder(IID_PPV_ARGS(&pDMLCommandRecorder)), std::cerr, false);

	return true;
}
