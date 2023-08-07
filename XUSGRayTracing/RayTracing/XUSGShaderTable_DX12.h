//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		class ShaderRecord_DX12 :
			public virtual ShaderRecord
		{
		public:
			ShaderRecord_DX12(const void* pShaderID, uint32_t shaderIDSize,
				const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0);
			ShaderRecord_DX12(const Device* pDevice, const Pipeline& pipeline, const wchar_t* shaderName,
				const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0);
			virtual ~ShaderRecord_DX12();

			void CopyTo(void* dest) const;

			static const void* GetShaderID(const Pipeline& pipeline, const wchar_t* shaderName);

			static uint32_t GetShaderIDSize(const Device* pDevice);

		protected:
			struct PointerWithSize
			{
				PointerWithSize() : Ptr(nullptr), Size(0) {}
				PointerWithSize(const void* ptr, uint32_t size) : Ptr(ptr), Size(size) {};

				const void* Ptr;
				uint32_t Size;
			};
			PointerWithSize m_shaderID;
			PointerWithSize m_localDescriptorArgs;
		};

		class ShaderTable_DX12 :
			public virtual ShaderTable
		{
		public:
			ShaderTable_DX12();
			virtual ~ShaderTable_DX12();

			bool Create(const XUSG::Device* pDevice, uint32_t numShaderRecords, uint32_t shaderRecordSize,
				const wchar_t* name = nullptr);

			bool AddShaderRecord(const ShaderRecord* pShaderRecord);

			void* Map();
			void Unmap();
			void Reset();

			const Resource* GetResource() const;
			uint32_t GetShaderRecordSize() const;

		protected:
			bool allocate(const XUSG::Device* pDevice, uint32_t byteWidth, const wchar_t* name);

			Resource::uptr m_resource;

			//std::vector<ShaderRecord> m_shaderRecords;

			void* m_mappedShaderRecords;
			uint32_t m_shaderRecordSize;
		};
	}
}
