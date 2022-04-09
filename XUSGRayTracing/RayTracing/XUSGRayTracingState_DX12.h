//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		class State_DX12 :
			public virtual State
		{
		public:
			struct KeyHeader
			{
				void* ShaderLib;
				void* GlobalPipelineLayout;
				uint32_t NumHitGroups;
				uint32_t NumLocalPipelineLayouts;
				uint32_t MaxPayloadSize;
				uint32_t MaxAttributeSize;
				uint32_t MaxRecursionDepth;
			};

			struct KeyHitGroup
			{
				const void* HitGroup;
				const void* ClosestHitShader;
				const void* AnyHitShader;
				const void* IntersectionShader;
				uint8_t Type;
			};

			struct KeyLocalPipelineLayoutHeader
			{
				void* PipelineLayout;
				uint32_t NumShaders;
			};

			struct KeyLocalPipelineLayout
			{
				KeyLocalPipelineLayoutHeader Header;
				std::vector<void*> Shaders;
			};

			State_DX12();
			virtual ~State_DX12();

			void SetShaderLibrary(Blob shaderLib);
			void SetHitGroup(uint32_t index, const void* hitGroup, const void* closestHitShader,
				const void* anyHitShader = nullptr, const void* intersectionShader = nullptr,
				HitGroupType type = HitGroupType::TRIANGLES);
			void SetShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize);
			void SetLocalPipelineLayout(uint32_t index, const XUSG::PipelineLayout& layout,
				uint32_t numShaders, const void** pShaders);
			void SetGlobalPipelineLayout(const XUSG::PipelineLayout& layout);
			void SetMaxRecursionDepth(uint32_t depth);

			Pipeline CreatePipeline(PipelineCache* pPipelineCache, const wchar_t* name = nullptr);
			Pipeline GetPipeline(PipelineCache* pPipelineCache, const wchar_t* name = nullptr);

			const std::string& GetKey();

		protected:
			void complete();

			KeyHeader* m_pKeyHeader;
			std::string m_key;

			std::vector<KeyHitGroup> m_keyHitGroups;
			std::vector<KeyLocalPipelineLayout> m_keyLocalPipelineLayouts;

			bool m_isComplete;
		};

		class PipelineCache_DX12 :
			public PipelineCache
		{
		public:
			PipelineCache_DX12();
			PipelineCache_DX12(const Device* pDevice);
			virtual ~PipelineCache_DX12();

			void SetDevice(const Device* pDevice);
			void SetPipeline(const std::string& key, const Pipeline& pipeline);

			Pipeline CreatePipeline(State* pState, const wchar_t* name = nullptr);
			Pipeline GetPipeline(State* pState, const wchar_t* name = nullptr);

		protected:
			Pipeline createPipeline(const std::string& key, const wchar_t* name);
			Pipeline getPipeline(const std::string& key, const wchar_t* name);

#if XUSG_ENABLE_DXR_FALLBACK
			com_ptr<ID3D12RaytracingFallbackDevice> m_device;

			std::unordered_map<std::string, com_ptr<ID3D12RaytracingFallbackStateObject>> m_stateObjects;
#else
			com_ptr<ID3D12Device5> m_device;

			std::unordered_map<std::string, com_ptr<ID3D12StateObject>> m_stateObjects;
#endif
		};
	}
}
