//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGTensor_DML.h"
#include "XUSGOperator_DML.h"
#include "XUSGBinding_DML.h"
#include "XUSGMachineLearning_DML.h"
#include "XUSGCommandRecorder_DML.h"
#include "XUSGMachineLearningUtil.h"

using namespace std;
using namespace XUSG;
using namespace XUSG::ML;

namespace
{
	void AssertMachineLearningApi()
	{
		XUSG::Device::LockApi();
		assert(XUSG::Device::GetApi() != API::VULKAN && "Machine learning is not implemented on Vulkan");
	}
}

Tensor::uptr Tensor::MakeUnique()
{
	AssertMachineLearningApi();

	return make_unique<Tensor_DML>();
}

Tensor::sptr Tensor::MakeShared()
{
	AssertMachineLearningApi();

	return make_shared<Tensor_DML>();
}

Operator* Operator::AsOperator()
{
	return static_cast<Operator*>(this);
}

Operator::uptr Operator::MakeUnique()
{
	AssertMachineLearningApi();

	return make_unique<Operator_DML>();
}

Operator::sptr Operator::MakeShared()
{
	AssertMachineLearningApi();

	return make_shared<Operator_DML>();
}

OperatorInitializer::uptr OperatorInitializer::MakeUnique()
{
	AssertMachineLearningApi();

	return make_unique<OperatorInitializer_DML>();
}

OperatorInitializer::sptr OperatorInitializer::MakeShared()
{
	AssertMachineLearningApi();

	return make_shared<OperatorInitializer_DML>();
}

Binding::uptr Binding::MakeUnique()
{
	AssertMachineLearningApi();

	return make_unique<Binding_DML>();
}

Binding::sptr Binding::MakeShared()
{
	AssertMachineLearningApi();

	return make_shared<Binding_DML>();
}

ML::Device::uptr ML::Device::MakeUnique()
{
	AssertMachineLearningApi();

	return make_unique<Device_DML>();
}

ML::Device::sptr ML::Device::MakeShared()
{
	AssertMachineLearningApi();

	return make_shared<Device_DML>();
}

CommandRecorder::uptr CommandRecorder::MakeUnique()
{
	AssertMachineLearningApi();

	return make_unique<CommandRecorder_DML>();
}

CommandRecorder::sptr CommandRecorder::MakeShared()
{
	AssertMachineLearningApi();

	return make_shared<CommandRecorder_DML>();
}

ML::Util::uptr ML::Util::MakeUnique(TensorDataType tensorDataType, TensorLayout tensorLayout)
{
	AssertMachineLearningApi();

	return make_unique<Util_Impl>(tensorDataType, tensorLayout);
}

ML::Util::sptr ML::Util::MakeShared(TensorDataType tensorDataType, TensorLayout tensorLayout)
{
	AssertMachineLearningApi();

	return make_shared<Util_Impl>(tensorDataType, tensorLayout);
}
