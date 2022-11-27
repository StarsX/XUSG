//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef _INDEPENDENT_SPHERICAL_HARMONICS_

#include "Core/XUSG.h"

namespace XUSG
{
	class XUSG_INTERFACE SphericalHarmonics
	{
	public:
		//SphericalHarmonics();
		virtual ~SphericalHarmonics() {}

		virtual bool Init(const Device* pDevice, const ShaderLib::sptr& shaderLib,
			const Compute::PipelineLib::sptr& computePipelineLib,
			const PipelineLayoutLib::sptr& pipelineLayoutLib,
			const DescriptorTableLib::sptr& descriptorTableLib,
			uint8_t baseCSIndex, uint8_t descriptorHeapIndex = 0) = 0;

		virtual void Transform(CommandList* pCommandList, Resource* pRadiance,
			const DescriptorTable& srvTable, uint8_t order = 3) = 0;

		virtual StructuredBuffer::sptr GetSHCoefficients() const = 0;

		virtual const DescriptorTable& GetSHCoeffSRVTable() const = 0;

		using uptr = std::unique_ptr<SphericalHarmonics>;
		using sptr = std::shared_ptr<SphericalHarmonics>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};
}
#else
#include "XUSGAdvanced.h"
#endif

namespace XUSG
{
	class SphericalHarmonics_Impl :
		public virtual SphericalHarmonics
	{
	public:
		SphericalHarmonics_Impl(API api);
		virtual ~SphericalHarmonics_Impl();

		bool Init(const Device* pDevice, const ShaderLib::sptr& shaderLib,
			const Compute::PipelineLib::sptr& computePipelineLib,
			const PipelineLayoutLib::sptr& pipelineLayoutLib,
			const DescriptorTableLib::sptr& descriptorTableLib,
			uint8_t baseCSIndex, uint8_t descriptorHeapIndex = 0);

		void Transform(CommandList* pCommandList, Resource* pRadiance,
			const DescriptorTable& srvTable, uint8_t order = 3);

		StructuredBuffer::sptr GetSHCoefficients() const;

		const DescriptorTable& GetSHCoeffSRVTable() const;

		static const uint8_t CubeMapFaceCount = 6;

	protected:
		enum DescriptorTableSlot : uint8_t
		{
			UAV_BUFFERS,
			SRV_BUFFERS,
			CONSTANTS
		};

		enum PipelineIndex : uint8_t
		{
			SH_CUBE_MAP,
			SH_SUM,
			SH_NORMALIZE,

			NUM_PIPELINE
		};

		enum UavSRVTableIndex : uint8_t
		{
			UAV_SRV_SH,
			UAV_SRV_SH1,

			NUM_UAV_SRV_TABLE
		};

		bool createPipelineLayouts();
		bool createPipelines();
		bool createDescriptorTables();

		void shCubeMap(CommandList* pCommandList, Resource* pRadiance,
			const DescriptorTable& srvTable, uint8_t order);
		void shSum(CommandList* pCommandList, uint8_t order);
		void shNormalize(CommandList* pCommandList, uint8_t order);

		API m_api;

		ShaderLib::sptr				m_shaderLib;
		Compute::PipelineLib::sptr	m_computePipelineLib;
		PipelineLayoutLib::sptr		m_pipelineLayoutLib;
		DescriptorTableLib::sptr	m_descriptorTableLib;

		PipelineLayout	m_pipelineLayouts[NUM_PIPELINE];
		Pipeline		m_pipelines[NUM_PIPELINE];

		StructuredBuffer::sptr	m_coeffSH[2];
		StructuredBuffer::uptr	m_weightSH[2];

		DescriptorTable	m_uavTables[NUM_UAV_SRV_TABLE];
		DescriptorTable	m_srvTables[NUM_UAV_SRV_TABLE];

		uint32_t	m_numSHTexels;
		uint8_t		m_baseCSIndex;
		uint8_t		m_descriptorHeapIndex;
		uint8_t		m_shBufferParity;
	};
}
