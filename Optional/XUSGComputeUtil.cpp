#include "XUSGComputeUtil.h"

using namespace std;
using namespace XUSG;

ComputeUtil::ComputeUtil()
{
	m_shaderLib = ShaderLib::MakeUnique();
	m_computePipelineLib = Compute::PipelineLib::MakeUnique();
	m_pipelineLayoutLib = PipelineLayoutLib::MakeUnique();
}

ComputeUtil::ComputeUtil(const Device* pDevice) :
	ComputeUtil()
{
	SetDevice(pDevice);
}

ComputeUtil::~ComputeUtil()
{
}

bool ComputeUtil::SetPrefixSum(CommandList* pCommandList, bool safeMode,
	const DescriptorTableLib::sptr& descriptorTableLib, TypedBuffer* pBuffer,
	vector<Resource::uptr>* pUploaders, Format format, uint32_t maxElementCount)
{
	const auto pDevice = pCommandList->GetDevice();

	if (maxElementCount > 1024 * 1024)
		assert(!"Error: maxElementCount should be no more than 1048576!");
	m_safeMode = safeMode;
	m_descriptorTableLib = descriptorTableLib;
	m_format = format;
	m_maxElementCount = maxElementCount;

	// Create resources
	m_counter = TypedBuffer::MakeUnique();
	XUSG_N_RETURN(m_counter->Create(pDevice, 1, sizeof(uint32_t), Format::R32_UINT,
		ResourceFlag::ALLOW_UNORDERED_ACCESS | ResourceFlag::DENY_SHADER_RESOURCE,
		MemoryType::DEFAULT, 0, nullptr, 1, nullptr, MemoryFlag::NONE,
		L"GlobalBarrierCounter"), false);

	if (pBuffer)
	{
		m_pBuffer = pBuffer;
		// Create a UAV table
		{
			const auto descriptorTable = Util::DescriptorTable::MakeUnique();
			descriptorTable->SetDescriptors(0, 1, &pBuffer->GetUAV());
			XUSG_X_RETURN(m_uavTables[UAV_TABLE_DATA], descriptorTable->GetCbvSrvUavTable(m_descriptorTableLib.get()), false);
		}

		// Append a counter UAV table
		{
			const auto descriptorTable = Util::DescriptorTable::MakeUnique();
			descriptorTable->SetDescriptors(0, 1, &m_counter->GetUAV());
			XUSG_X_RETURN(m_uavTables[UAV_TABLE_COUNTER], descriptorTable->GetCbvSrvUavTable(m_descriptorTableLib.get()), false);
		}
	}
	else
	{
		// Select pipeline index and stride
		uint32_t stride = sizeof(uint32_t);
		switch (format)
		{
		case Format::R16_UINT:
		case Format::R16_SINT:
		case Format::R16_FLOAT:
			stride = sizeof(uint16_t);
			break;
		}

		// Create test buffers
		m_testBuffer = TypedBuffer::MakeUnique();
		XUSG_N_RETURN(m_testBuffer->Create(pDevice, maxElementCount, stride, format,
			ResourceFlag::ALLOW_UNORDERED_ACCESS | ResourceFlag::DENY_SHADER_RESOURCE,
			MemoryType::DEFAULT, 0, nullptr, 1, nullptr, MemoryFlag::NONE,
			L"PrefixSumTestBuffer"), false);

		m_readBack = TypedBuffer::MakeUnique();
		XUSG_N_RETURN(m_readBack->Create(pDevice, maxElementCount, stride, format,
			ResourceFlag::DENY_SHADER_RESOURCE, MemoryType::READBACK, 0,
			nullptr, 0, nullptr, MemoryFlag::NONE, L"ReadBackBuffer"), false);

		m_pBuffer = m_testBuffer.get();

		// Create a UAV table
		{
			const auto descriptorTable = Util::DescriptorTable::MakeUnique();
			descriptorTable->SetDescriptors(0, 1, &m_testBuffer->GetUAV(), TEMPORARY_HEAP);
			XUSG_X_RETURN(m_uavTables[UAV_TABLE_DATA], descriptorTable->GetCbvSrvUavTable(m_descriptorTableLib.get()), false);
		}

		// Append a counter UAV table
		{
			const auto descriptorTable = Util::DescriptorTable::MakeUnique();
			descriptorTable->SetDescriptors(0, 1, &m_counter->GetUAV(), TEMPORARY_HEAP);
			XUSG_X_RETURN(m_uavTables[UAV_TABLE_COUNTER], descriptorTable->GetCbvSrvUavTable(m_descriptorTableLib.get()), false);
		}

		// Upload test data
		m_testData.resize(stride * maxElementCount);
		const auto pTestData = m_testData.data();
		switch (format)
		{
		case Format::R32_SINT:
			for (auto i = 0u; i < maxElementCount; ++i)
				reinterpret_cast<int32_t*>(pTestData)[i] = (rand() & 1) ? -rand() : rand();
			break;
		case Format::R16_SINT:
			for (auto i = 0u; i < maxElementCount; ++i)
				reinterpret_cast<int16_t*>(pTestData)[i] = (rand() & 1) ? -rand() : rand();
			break;
		case Format::R8_SINT:
		case Format::R8_UINT:
			for (auto i = 0u; i < maxElementCount; ++i)
				reinterpret_cast<uint8_t*>(pTestData)[i] = rand() & 0xff;
			break;
		case Format::R16_UINT:
			for (auto i = 0u; i < maxElementCount; ++i)
				reinterpret_cast<uint16_t*>(pTestData)[i] = rand();
			break;
		case Format::R32_FLOAT:
			for (auto i = 0u; i < maxElementCount; ++i)
				reinterpret_cast<float*>(pTestData)[i] = rand() / 1000.0f;
			break;
		default:
			for (auto i = 0u; i < maxElementCount; ++i)
				reinterpret_cast<uint32_t*>(pTestData)[i] = rand();
		}

		if (!pUploaders) assert(!"Error: if pBufferView is nullptr, pUploaders must not be nullptr!");
		pUploaders->emplace_back(Resource::MakeUnique());
		m_testBuffer->Upload(pCommandList, pUploaders->back().get(), m_testData.data(),
			stride * maxElementCount, 0, ResourceState::UNORDERED_ACCESS);
	}

	// Select pipeline index and stride
	auto pipelineIndex = safeMode ? PREFIX_SUM_UINT1 : PREFIX_SUM_UINT;
	switch (format)
	{
	case Format::R32_SINT:
	case Format::R16_SINT:
	case Format::R8_SINT:
		pipelineIndex = safeMode ? PREFIX_SUM_SINT1 : PREFIX_SUM_SINT;
		break;
	case Format::R32_FLOAT:
	case Format::R16_FLOAT:
		pipelineIndex = safeMode ? PREFIX_SUM_FLOAT1 : PREFIX_SUM_FLOAT;
		break;
	}

	// Create pipeline layout
	const auto pipelineLayout = Util::PipelineLayout::MakeUnique();
	pipelineLayout->SetConstants(0, XUSG_UINT32_SIZE_OF(uint32_t[2]), 0);
	pipelineLayout->SetRange(1, DescriptorType::UAV, 2, 0, 0,
		DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE | DescriptorFlag::DESCRIPTORS_VOLATILE);
	XUSG_X_RETURN(m_pipelineLayouts[pipelineIndex], pipelineLayout->GetPipelineLayout(m_pipelineLayoutLib.get(),
		PipelineLayoutFlag::NONE, L"PrefixSumLayout"), false);

	// Create pipeline
	if (!m_pipelines[pipelineIndex])
	{
		XUSG_N_RETURN(m_shaderLib->CreateShader(Shader::Stage::CS, pipelineIndex,
			safeMode ? L"CSPrefixSum1.cso" : L"CSPrefixSum.cso"), false);

		const auto state = Compute::State::MakeUnique();
		state->SetPipelineLayout(m_pipelineLayouts[pipelineIndex]);
		state->SetShader(m_shaderLib->GetShader(Shader::Stage::CS, pipelineIndex));
		XUSG_X_RETURN(m_pipelines[pipelineIndex], state->GetPipeline(m_computePipelineLib.get(),
			safeMode ? L"PrefixSum1" : L"PrefixSum"), false);
	}

	if (safeMode)
	{
		// Create pipeline layout
		const auto pipelineLayout = Util::PipelineLayout::MakeUnique();
		pipelineLayout->SetRange(0, DescriptorType::UAV, 1, 0, 0,
			DescriptorFlag::DATA_STATIC_WHILE_SET_AT_EXECUTE | DescriptorFlag::DESCRIPTORS_VOLATILE);
		XUSG_X_RETURN(m_pipelineLayouts[pipelineIndex + 1], pipelineLayout->GetPipelineLayout(m_pipelineLayoutLib.get(),
			PipelineLayoutFlag::NONE, L"PrefixSum2Layout"), false);

		// Create pipeline
		if (!m_pipelines[pipelineIndex + 1])
		{
			XUSG_N_RETURN(m_shaderLib->CreateShader(Shader::Stage::CS, pipelineIndex + 1, L"CSPrefixSum2.cso"), false);

			const auto state = Compute::State::MakeUnique();
			state->SetPipelineLayout(m_pipelineLayouts[pipelineIndex + 1]);
			state->SetShader(m_shaderLib->GetShader(Shader::Stage::CS, pipelineIndex + 1));
			XUSG_X_RETURN(m_pipelines[pipelineIndex + 1], state->GetPipeline(m_computePipelineLib.get(), L"PrefixSum2"), false);
		}
	}

	return true;
}

void ComputeUtil::SetDevice(const Device* pDevice)
{
	m_computePipelineLib->SetDevice(pDevice);
	m_pipelineLayoutLib->SetDevice(pDevice);
}

void ComputeUtil::PrefixSum(CommandList* pCommandList, uint32_t numElements)
{
	numElements = numElements != UINT32_MAX ? numElements : m_maxElementCount;
	if (m_testBuffer && numElements > m_maxElementCount)
			assert(!"Error: numElements is greater than maxElementCount!");

	const auto descriptorHeap = m_descriptorTableLib->GetDescriptorHeap(CBV_SRV_UAV_HEAP,
			m_testBuffer ? TEMPORARY_HEAP : PERMANENT_HEAP);
	pCommandList->SetDescriptorHeaps(1, &descriptorHeap);

	// Clear counter
	const uint32_t clear[4] = {};
	pCommandList->ClearUnorderedAccessViewUint(m_uavTables[UAV_TABLE_COUNTER],
		m_counter->GetUAV(), m_counter.get(), clear);

	// Select pipeline index
	auto pipelineIndex = m_safeMode ? PREFIX_SUM_UINT1 : PREFIX_SUM_UINT;
	switch (m_format)
	{
	case Format::R32_SINT:
	case Format::R16_SINT:
	case Format::R8_SINT:
		pipelineIndex = m_safeMode ? PREFIX_SUM_SINT1 : PREFIX_SUM_SINT;
		break;
	case Format::R32_FLOAT:
	case Format::R16_FLOAT:
		pipelineIndex = m_safeMode ? PREFIX_SUM_FLOAT1 : PREFIX_SUM_FLOAT;
		break;
	}

	// Set pipeline state
	pCommandList->SetComputePipelineLayout(m_pipelineLayouts[pipelineIndex]);
	pCommandList->SetPipelineState(m_pipelines[pipelineIndex]);

	// Set descriptor tables
	const auto groupSize = 1024u;
	const auto numGroups = XUSG_DIV_UP(numElements, groupSize);
	const auto remainder = numElements & (groupSize - 1);
	pCommandList->SetCompute32BitConstant(0, numGroups);
	pCommandList->SetCompute32BitConstant(0, remainder, 1);
	pCommandList->SetComputeDescriptorTable(1, m_uavTables[UAV_TABLE_DATA]);

	pCommandList->Dispatch(numGroups, 1, 1);

	if (m_safeMode)
	{
		// Set barrier
		ResourceBarrier barrier;
		const auto numBarriers = m_pBuffer->SetBarrier(&barrier, ResourceState::UNORDERED_ACCESS);
		pCommandList->Barrier(numBarriers, &barrier);

		// Set pipeline state
		pCommandList->SetComputePipelineLayout(m_pipelineLayouts[pipelineIndex + 1]);
		pCommandList->SetPipelineState(m_pipelines[pipelineIndex + 1]);

		// Set descriptor tables
		pCommandList->SetComputeDescriptorTable(0, m_uavTables[UAV_TABLE_DATA]);

		pCommandList->Dispatch(XUSG_DIV_UP(numElements, 64), 1, 1);
	}

	if (m_readBack)
	{
		assert(m_testBuffer);

		// Set barrier
		ResourceBarrier barrier;
		m_testBuffer->SetBarrier(&barrier, ResourceState::UNORDERED_ACCESS); // Promotion
		const auto numBarriers = m_testBuffer->SetBarrier(&barrier, ResourceState::COPY_SOURCE);
		pCommandList->Barrier(numBarriers, &barrier);

		// Copy the counter for readback
		pCommandList->CopyResource(m_readBack.get(), m_testBuffer.get());
	}
}

void ComputeUtil::VerifyPrefixSum(uint32_t numElements)
{
	// Generate ground truth
#define GENERATE_GROUND_TRUTH(T) \
	vector<T> groundTruths(numElements); \
	for (size_t i = 1; i < numElements; ++i) \
		groundTruths[i] = groundTruths[i - 1] + reinterpret_cast<const T*>(pTestData)[i - 1]

#ifndef COMPARE
#define COMPARE(T) COMPARE_AND_SHOW(T, false)
#endif

	// Verify results
#define COMPARE_AND_SHOW(T, A) \
	const auto testResults = reinterpret_cast<const T*>(m_readBack->Map(nullptr)); \
	for (size_t i = 0; i < numElements; ++i) \
	{ \
		if (testResults[i] != groundTruths[i]) \
			cout << "Wrong " << i << ": input (" << \
			reinterpret_cast<const T*>(pTestData)[i] << \
			"), result (" << testResults[i] << "), ground truth (" << \
			groundTruths[i] << ")" << endl; \
		else if (A)\
			cout << "Correct " << i << ": input (" << \
			reinterpret_cast<const T*>(pTestData)[i] << \
			"), result (" << testResults[i] << "), ground truth (" << \
			groundTruths[i] << ")" << endl; \
		if (i % 1024 == 1023 && A) system("pause"); \
	}

#define VERIFY(T) GENERATE_GROUND_TRUTH(T); COMPARE(T)

	numElements = numElements != UINT32_MAX ? numElements : m_maxElementCount;

	const auto pTestData = m_testData.data();
	switch (m_format)
	{
	case Format::R32_SINT:
	{
		VERIFY(int32_t);
		break;
	}
	case Format::R16_SINT:
	{
		VERIFY(int16_t);
		break;
	}
	case Format::R8_SINT:
	{
		VERIFY(int8_t);
		break;
	}
	case Format::R8_UINT:
	{
		VERIFY(uint8_t);
		break;
	}
	case Format::R16_UINT:
	{
		VERIFY(uint16_t);
		break;
	}
	case Format::R32_FLOAT:
	{
		VERIFY(float);
		break;
	}
	default:
	{
		VERIFY(uint32_t);
	}
	}

	m_readBack->Unmap();

#undef VERIFY
#undef COMPARE_AND_SHOW
#undef COMPARE
#undef GENERATE_GROUND_TRUTH
}
