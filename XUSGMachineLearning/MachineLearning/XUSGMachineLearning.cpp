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

Tensor::uptr Tensor::MakeUnique(XUSG::API api)
{
	return make_unique<Tensor_DML>();
}

Tensor::sptr Tensor::MakeShared(XUSG::API api)
{
	return make_shared<Tensor_DML>();
}

Operator* Operator::AsOperator()
{
	return static_cast<Operator*>(this);
}

Operator::uptr Operator::MakeUnique(XUSG::API api)
{
	return make_unique<Operator_DML>();
}

Operator::sptr Operator::MakeShared(XUSG::API api)
{
	return make_shared<Operator_DML>();
}

OperatorInitializer::uptr OperatorInitializer::MakeUnique(XUSG::API api)
{
	return make_unique<OperatorInitializer_DML>();
}

OperatorInitializer::sptr OperatorInitializer::MakeShared(XUSG::API api)
{
	return make_shared<OperatorInitializer_DML>();
}

Binding::uptr Binding::MakeUnique(XUSG::API api)
{
	return make_unique<Binding_DML>();
}

Binding::sptr Binding::MakeShared(XUSG::API api)
{
	return make_shared<Binding_DML>();
}

ML::Device::uptr ML::Device::MakeUnique(XUSG::API api)
{
	return make_unique<Device_DML>();
}

ML::Device::sptr ML::Device::MakeShared(XUSG::API api)
{
	return make_shared<Device_DML>();
}

CommandRecorder::uptr CommandRecorder::MakeUnique(XUSG::API api)
{
	return make_unique<CommandRecorder_DML>();
}

CommandRecorder::sptr CommandRecorder::MakeShared(XUSG::API api)
{
	return make_shared<CommandRecorder_DML>();
}

ML::Util::uptr ML::Util::MakeUnique(const ML::Device::sptr& device, TensorDataType tensorDataType,
	TensorLayout tensorLayout, XUSG::API api)
{
	return make_unique<Util_Impl>(device, tensorDataType, tensorLayout, api);
}

ML::Util::sptr ML::Util::MakeShared(const ML::Device::sptr& device, TensorDataType tensorDataType,
	TensorLayout tensorLayout, XUSG::API api)
{
	return make_shared<Util_Impl>(device, tensorDataType, tensorLayout, api);
}
