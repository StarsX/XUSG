//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGRayTracing.h"

namespace XUSG
{
	namespace RayTracing
	{
		void CacheShaderRecordAlignment(uint32_t identifierSize, uint32_t recordAlignment, uint32_t tableAlignment);

		class ShaderRecord_VK :
			public virtual ShaderRecord
		{
		public:
			ShaderRecord_VK(const void* pShaderIdentifier, uint32_t shaderIdentifierSize,
				const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0);
			ShaderRecord_VK(Device* pDevice, const Pipeline& pipeline, const wchar_t* shaderName,
				const void* pLocalDescriptorArgs = nullptr, uint32_t localDescriptorArgSize = 0);
			virtual ~ShaderRecord_VK();

			void CopyTo(void* dest) const;

			static const void* GetShaderIdentifier(const Pipeline& pipeline, const wchar_t* shaderName);

			static uint32_t GetShaderIdentifierSize(Device* pDevice);

			static size_t Align(uint32_t byteSize);

		protected:
			struct PointerWithSize
			{
				PointerWithSize() : Ptr(nullptr), Size(0) {}
				PointerWithSize(const void* ptr, uint32_t size) : Ptr(ptr), Size(size) {};

				const void* Ptr;
				uint32_t Size;
			};
			PointerWithSize m_shaderIdentifier;
			PointerWithSize m_localDescriptorArgs;
		};

		class ShaderTable_VK :
			public virtual ShaderTable
		{
		public:
			ShaderTable_VK();
			virtual ~ShaderTable_VK();

			bool Create(XUSG::Device* pDevice, uint32_t numShaderRecords, uint32_t shaderRecordSize,
				MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr);

			void Create(Buffer::sptr resource, uint32_t shaderRecordSize, uintptr_t byteOffset);
			void AddShaderRecord(const ShaderRecord* pShaderRecord);
			void Reset();

			uint64_t GetVirtualAddress() const;
			size_t GetByteSize() const;
			size_t GetByteStride() const;

			static size_t Align(size_t byteSize);

		protected:
			Buffer::sptr m_resource;
			uintptr_t m_byteOffset;
			size_t m_byteSize;
			size_t m_byteStride;

			uint8_t* m_mappedData;
		};
	}
}
