//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"

namespace XUSG
{
	class ComputeUtil
	{
	public:
		ComputeUtil();
		ComputeUtil(const Device* pDevice);
		virtual ~ComputeUtil();

		bool SetPrefixSum(CommandList* pCommandList, bool safeMode,
			const DescriptorTableLib::sptr& descriptorTableLib,
			TypedBuffer* pBuffer, std::vector<Resource::uptr>* pUploaders = nullptr,
			Format format = Format::R32_UINT, uint32_t maxElementCount = 4096);

		void SetDevice(const Device* pDevice);
		void PrefixSum(CommandList* pCommandList, uint32_t numElements = UINT32_MAX);
		void VerifyPrefixSum(uint32_t numElements = UINT32_MAX);

	protected:
		enum PipelineIndex : uint8_t
		{
			PREFIX_SUM_UINT,
			PREFIX_SUM_SINT,
			PREFIX_SUM_FLOAT,

			PREFIX_SUM_UINT1,
			PREFIX_SUM_UINT2,
			PREFIX_SUM_SINT1,
			PREFIX_SUM_SINT2,
			PREFIX_SUM_FLOAT1,
			PREFIX_SUM_FLOAT2,

			NUM_PIPELINE
		};

		enum DescriptorHeapIndex : uint8_t
		{
			PERMANENT_HEAP,
			TEMPORARY_HEAP
		};

		enum UAVTable : uint8_t
		{
			UAV_TABLE_DATA,
			UAV_TABLE_COUNTER,

			NUM_UAV_TABLE
		};

		Format m_format;

		XUSG::ShaderLib::uptr				m_shaderLib;
		XUSG::Compute::PipelineLib::uptr	m_computePipelineLib;
		XUSG::PipelineLayoutLib::uptr		m_pipelineLayoutLib;
		XUSG::DescriptorTableLib::sptr		m_descriptorTableLib;

		PipelineLayout			m_pipelineLayouts[NUM_PIPELINE];
		Pipeline				m_pipelines[NUM_PIPELINE];

		TypedBuffer::uptr		m_counter;
		TypedBuffer::uptr		m_testBuffer;
		TypedBuffer::uptr		m_readBack;
		TypedBuffer*			m_pBuffer;

		DescriptorTable			m_uavTables[NUM_UAV_TABLE];

		std::vector<uint8_t>	m_testData;

		bool					m_safeMode;
		uint32_t				m_maxElementCount;
	};
}
