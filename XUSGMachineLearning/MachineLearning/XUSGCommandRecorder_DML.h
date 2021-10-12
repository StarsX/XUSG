//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGMachineLearning.h"

namespace XUSG
{
	namespace ML
	{
		class CommandRecorder_DML :
			public virtual CommandRecorder
		{
		public:
			CommandRecorder_DML();
			virtual ~CommandRecorder_DML();

			bool Create(const Device* pDevice, const wchar_t* name = nullptr);

			void Dispatch(XUSG::CommandList* pCommandList, const Dispatchable& dispatchable, const BindingTable& bindings) const;

			void* GetHandle() const;

			com_ptr<IDMLCommandRecorder>& GetDMLCommandRecorder();

		protected:
			com_ptr<IDMLCommandRecorder> m_commandRecorder;
		};
	}
}
