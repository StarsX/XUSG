//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef _INDEPENDENT_DDS_LOADER_
namespace XUSG
{
	namespace DDS
	{
		enum AlphaMode : uint8_t
		{
			ALPHA_MODE_UNKNOWN,
			ALPHA_MODE_STRAIGHT,
			ALPHA_MODE_PREMULTIPLIED,
			ALPHA_MODE_OPAQUE,
			ALPHA_MODE_CUSTOM
		};

		class DLL_INTERFACE Loader
		{
		public:
			Loader();
			virtual ~Loader();

			bool CreateTextureFromMemory(CommandList* pCommandList, const uint8_t* ddsData,
				size_t ddsDataSize, size_t maxsize, bool forceSRGB, Texture::sptr& texture, Resource* pUploader,
				AlphaMode* alphaMode = nullptr, ResourceState state = ResourceState::COMMON,
				MemoryFlag memoryFlags = MemoryFlag::NONE, API api = API::DIRECTX_12);

			bool CreateTextureFromFile(CommandList* pCommandList, const wchar_t* fileName,
				size_t maxsize, bool forceSRGB, Texture::sptr& texture, Resource* pUploader,
				AlphaMode* alphaMode = nullptr, ResourceState state = ResourceState::COMMON,
				MemoryFlag memoryFlags = MemoryFlag::NONE, API api = API::DIRECTX_12);

			static size_t BitsPerPixel(Format fmt);
		};
	}
}
#else
#include "XUSGAdvanced.h"
#endif
