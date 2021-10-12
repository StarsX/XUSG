//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Core/XUSG_DX12.h"
#include "Core/XUSGCommand_DX12.h"
#include "XUSGCommandRecorder_DML.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::ML;

CommandRecorder_DML::CommandRecorder_DML()
{
}

CommandRecorder_DML::~CommandRecorder_DML()
{
}

bool CommandRecorder_DML::Create(const Device* pDevice, const wchar_t* name)
{
	const auto pDMLDevice = static_cast<IDMLDevice*>(pDevice->GetHandle());

	assert(pDMLDevice);
	V_RETURN(pDMLDevice->CreateCommandRecorder(IID_PPV_ARGS(&m_commandRecorder)), cerr, false);
	if (name) m_commandRecorder->SetName(name);

	return true;
}

void CommandRecorder_DML::Dispatch(CommandList* pCommandList, const Dispatchable& dispatchable, const BindingTable& bindings) const
{
	m_commandRecorder->RecordDispatch(static_cast<ID3D12CommandList*>(pCommandList->GetHandle()),
		static_cast<IDMLDispatchable*>(dispatchable), static_cast<IDMLBindingTable*>(bindings));
}

void* CommandRecorder_DML::GetHandle() const
{
	return m_commandRecorder.get();
}

com_ptr<IDMLCommandRecorder>& CommandRecorder_DML::GetDMLCommandRecorder()
{
	return m_commandRecorder;
}
