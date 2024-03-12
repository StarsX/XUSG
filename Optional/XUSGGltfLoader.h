//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <map>

namespace XUSG
{
	class GltfLoader
	{
	public:
		enum AlphaMode : uint8_t
		{
			ALPHA_OPAQUE,
			ALPHA_MASK,
			ALPHA_BLEND
		};

		struct float2
		{
			float x;
			float y;

			float2() = default;
			constexpr float2(float _x, float _y) : x(_x), y(_y) {}
			explicit float2(const float* pArray) : x(pArray[0]), y(pArray[1]) {}
			explicit float2(float v) : x(v), y(v) {}

			float2& operator= (const float2& v) { x = v.x; y = v.y; return *this; }
		};

		struct float3
		{
			float x;
			float y;
			float z;

			float3() = default;
			constexpr float3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
			explicit float3(const float* pArray) : x(pArray[0]), y(pArray[1]), z(pArray[2]) {}
			explicit float3(float v) : x(v), y(v), z(v) {}

			float3& operator= (const float3& v) { x = v.x; y = v.y; z = v.z; return *this; }
		};

		struct float4
		{
			float x;
			float y;
			float z;
			float w;

			float4() = default;
			constexpr float4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
			explicit float4(const float* pArray) : x(pArray[0]), y(pArray[1]), z(pArray[2]), w(pArray[3]) {}
			explicit float4(float v) : x(v), y(v), z(v), w(v) {}
			explicit float4(float v, float _w) : x(v), y(v), z(v), w(_w) {}

			float4& operator= (const float4& v) { x = v.x; y = v.y; z = v.z; w = v.w; return *this; }
		};

		struct Texture
		{
			uint32_t Width;
			uint32_t Height;
			uint8_t Channels;
			std::vector<uint8_t> Data;
		};

		struct Subset
		{
			uint32_t IndexOffset;
			uint32_t NumIndices;
			uint32_t BaseColorTexIdx;
			uint32_t NormalTexIdx;
			uint32_t MtlRghTexIdx;
			AlphaMode AlphaMode;
			float2 LightMapScl;
		};

		struct LightSource
		{
			float4 Min;
			float4 Max;
			float4 Emissive;
		};

		struct AABB
		{
			float3 Min;
			float3 Max;
		}; 

		GltfLoader();
		virtual ~GltfLoader();

		bool Import(const char* pszFilename, bool needNorm = true, bool needColor = true,
			bool needBound = true, bool invertZ = true);

		const uint32_t GetNumVertices() const;
		const uint32_t GetNumIndices() const;
		const uint32_t GetNumSubSets() const;
		const uint32_t GetNumTextures() const;
		const uint32_t GetVertexStride() const;
		const uint8_t* GetVertices() const;
		const uint32_t* GetIndices() const;
		const Subset* GetSubsets() const;
		const Texture* GetTextures() const;

		const AABB& GetAABB() const;

		const std::vector<LightSource>& GetLightSources() const;

	protected:
		void fillVertexColors(uint32_t offset, uint32_t size, float4 color);
		void fillVertexScalars(uint32_t offset, uint32_t size, float scalar);
		void recomputeNormals();
		void computeAABB();
		void regenerateUV1(uint32_t vertexOffset, uint32_t texcoordCount, bool useInputMeshUvs);

		uint8_t* getVertex(uint32_t i);
		float3& getPosition(uint32_t i);
		float3& getNormal(uint32_t i);
		float4& getTexcoord(uint32_t i);
		float4& getTangent(uint32_t i);
		uint32_t& getVertexColor(uint32_t i);
		float& getVertexScalar(uint32_t i);

		uint32_t float4_to_rgba8(const float4& f);
		float saturateFloat(float f);
		uint32_t float_to_uint(float f, float scale);

		std::vector<uint8_t>	m_vertices;
		std::vector<uint32_t>	m_indices;
		std::vector<Subset>		m_subsets;
		std::vector<LightSource> m_lightSources;

		std::vector<Texture>	m_textures;
		std::map<void*, uint32_t> m_texIndexMap;

		uint32_t	m_stride;
		uint32_t	m_posOffset;
		uint32_t	m_nrmOffset;
		uint32_t	m_txcOffset;
		uint32_t	m_tanOffset;
		uint32_t	m_colorOffset;
		uint32_t	m_scalarOffset;

		AABB m_aabb;
	};
}
