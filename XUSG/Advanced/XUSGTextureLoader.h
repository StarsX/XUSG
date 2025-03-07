//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef _ENABLE_STB_IMAGE_LOADER_ONLY_
#define _ENABLE_STB_IMAGE_LOADER_
#define _DISABLE_DDS_LOADER_
#endif

#ifdef _ENABLE_STB_IMAGE_LOADER_
#include "stb_image.h"

namespace XUSG
{
	stbi_uc* LoadImageFromFile(const char* fileName, int& width, int& height, int& reqChannels, XUSG::Format& format)
	{
		int channels;
		const auto infoStat = stbi_info(fileName, &width, &height, &channels);
		assert(infoStat);
		reqChannels = channels != 3 ? channels : 4;

		switch (reqChannels)
		{
		case 1:
			format = Format::R8_UNORM;
			break;
		case 2:
			format = Format::R8G8_UNORM;
			break;
		case 4:
			format = Format::R8G8B8A8_UNORM;
			break;
		default:
			assert(!"Wrong channels, unknown format!");
		}

		return stbi_load(fileName, &width, &height, &channels, reqChannels);
	}

	bool CreateTextureFromFile(CommandList* pCommandList, const char* fileName,
		Texture* pTexture, Resource* pUploader, ResourceState state = ResourceState::COMMON,
		MemoryFlag memoryFlags = MemoryFlag::NONE, const wchar_t* name = nullptr)
	{
		Format format;
		int width, height, reqChannels;
		const auto pTexData = LoadImageFromFile(fileName, width, height, reqChannels, format);

		XUSG_N_RETURN(pTexture->Create(pCommandList->GetDevice(), width, height, format, 1,
			ResourceFlag::NONE, 1, 1, false, memoryFlags, name), false);

		XUSG_N_RETURN(pTexture->Upload(pCommandList, pUploader, pTexData, reqChannels, state), false);
		free(pTexData);

		return true;
	}
}
#endif

#ifndef _DISABLE_DDS_LOADER_
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

		class XUSG_INTERFACE Loader
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
#endif
