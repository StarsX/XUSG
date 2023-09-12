//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace XUSG
{
	class ObjLoader
	{
	public:
		struct float3
		{
			float x;
			float y;
			float z;

			float3() = default;
			constexpr float3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
			explicit float3(const float* pArray) : x(pArray[0]), y(pArray[1]), z(pArray[2]) {}

			float3& operator= (const float3& Float3) { x = Float3.x; y = Float3.y; z = Float3.z; return *this; }
		};

		struct AABB
		{
			float3 Min;
			float3 Max;
		};

		ObjLoader();
		virtual ~ObjLoader();

		bool Import(const char* pszFilename, bool needNorm = true,
			bool needAABB = true, bool forDX = true, bool swapYZ = false);

		const uint32_t GetNumVertices() const;
		const uint32_t GetNumIndices() const;
		const uint32_t GetVertexStride() const;
		const uint8_t* GetVertices() const;
		const uint32_t* GetIndices() const;

		const AABB& GetAABB() const;

	protected:
		void importGeometryFirstPass(FILE* pFile, uint32_t& numTexc, uint32_t& numNorm);
		void importGeometrySecondPass(FILE* pFile, uint32_t numTexc, uint32_t numNorm, bool forDX, bool swapYZ);
		void loadIndices(FILE* pFile, uint32_t& numTri, uint32_t numTexc, uint32_t numNorm,
			std::vector<uint32_t>& nIndices, std::vector<uint32_t>& tIndices);
		void computePerVertexNormals(const std::vector<float3>& normals, const std::vector<uint32_t>& nIndices);
		void recomputeNormals();
		void computeAABB();

		void* getVertex(uint32_t i);
		float3& getPosition(uint32_t i);
		float3& getNormal(uint32_t i);

		std::vector<uint8_t>	m_vertices;
		std::vector<uint32_t>	m_indices;

		uint32_t	m_stride;

		AABB		m_aabb;
	};
}
