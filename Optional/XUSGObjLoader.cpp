//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGObjLoader.h"

using namespace std;
using namespace XUSG;

ObjLoader::ObjLoader()
{
}

ObjLoader::~ObjLoader()
{
}

bool ObjLoader::Import(const char* pszFilename, bool needNorm, bool needAABB, bool forDX, bool swapYZ)
{
	FILE* pFile;
	fopen_s(&pFile, pszFilename, "r");

	if (!pFile) return false;

	m_stride = sizeof(float3);
	m_stride += needNorm ? sizeof(float3) : 0;

	// Import the OBJ file.
	uint32_t numTexc, numNorm;
	importGeometryFirstPass(pFile, numTexc, numNorm);
	rewind(pFile);
	importGeometrySecondPass(pFile, numTexc, numNorm, forDX, swapYZ);
	fclose(pFile);

	// Perform post import tasks.
	if (needNorm && !numNorm) recomputeNormals();
	if (needAABB) computeAABB();

	return true;
}

const uint32_t ObjLoader::GetNumVertices() const
{
	return static_cast<uint32_t>(m_vertices.size() / GetVertexStride());
}

const uint32_t ObjLoader::GetNumIndices() const
{
	return static_cast<uint32_t>(m_indices.size());
}

const uint32_t ObjLoader::GetVertexStride() const
{
	return m_stride;
}

const uint8_t* ObjLoader::GetVertices() const
{
	return m_vertices.data();
}

const uint32_t* ObjLoader::GetIndices() const
{
	return m_indices.data();
}

const ObjLoader::AABB& ObjLoader::GetAABB() const
{
	return m_aabb;
}

void ObjLoader::importGeometryFirstPass(FILE* pFile, uint32_t& numTexc, uint32_t& numNorm)
{
	auto v = 0u;
	auto vt = 0u;
	auto vn = 0u;
	char buffer[256] = { 0 };

	auto numVert = 0u;
	auto numTri = 0u;
	numTexc = 0;
	numNorm = 0;

	while (fscanf_s(pFile, "%s", buffer, static_cast<uint32_t>(sizeof(buffer))) != EOF)
	{
		switch (buffer[0])
		{
		case 'f':   // v, v//vn, v/vt, v/vt/vn.
			fscanf_s(pFile, "%s", buffer, static_cast<uint32_t>(sizeof(buffer)));

			if (strstr(buffer, "//")) // v//vn
			{
				sscanf_s(buffer, "%u//%u", &v, &vn);
				fscanf_s(pFile, "%u//%u", &v, &vn);
				fscanf_s(pFile, "%u//%u", &v, &vn);
				++numTri;

				while (fscanf_s(pFile, "%u//%u", &v, &vn) > 0) ++numTri;
			}
			else if (sscanf_s(buffer, "%u/%u/%u", &v, &vt, &vn) == 3) // v/vt/vn
			{
				fscanf_s(pFile, "%u/%u/%u", &v, &vt, &vn);
				fscanf_s(pFile, "%u/%u/%u", &v, &vt, &vn);
				++numTri;

				while (fscanf_s(pFile, "%u/%u/%u", &v, &vt, &vn) > 0) ++numTri;

				//m_hasTexcoord = true;
			}
			else if (sscanf_s(buffer, "%u/%u", &v, &vt) == 2) // v/vt
			{
				fscanf_s(pFile, "%u/%u", &v, &vt);
				fscanf_s(pFile, "%u/%u", &v, &vt);
				++numTri;

				while (fscanf_s(pFile, "%u/%u", &v, &vt) > 0) ++numTri;

				//m_hasTexcoord = true;
			}
			else // v
			{
				fscanf_s(pFile, "%u", &v);
				fscanf_s(pFile, "%u", &v);
				++numTri;

				while (fscanf_s(pFile, "%u", &v) > 0)
					++numTri;
			}
			break;

		case 'v':   // v, vt, or vn
			switch (buffer[1])
			{
			case '\0':
				fgets(buffer, sizeof(buffer), pFile);
				++numVert;
				break;
			case 't':
				fgets(buffer, sizeof(buffer), pFile);
				++numTexc;
				break;
			case 'n':
				fgets(buffer, sizeof(buffer), pFile);
				++numNorm;
				break;
			default:
				break;
			}
			break;

		default:
			fgets(buffer, sizeof(buffer), pFile);
			break;
		}
	}

	// Allocate memory for the OBJ model data.
	const auto numIdx = numTri * 3;
	m_stride += m_stride <= sizeof(float3) && numNorm ? sizeof(float3) : 0;
	m_stride += numTexc ? sizeof(float[2]) : 0;
	m_vertices.reserve(m_stride * (max)((max)(numVert, numTexc), numNorm));
	m_vertices.resize(m_stride * numVert);
	m_indices.resize(numIdx);
}

void ObjLoader::importGeometrySecondPass(FILE* pFile, uint32_t numTexc, uint32_t numNorm, bool forDX, bool swapYZ)
{
	auto numVert = 0u;
	auto numTri = 0u;
	char buffer[256] = { 0 };

	vector<float3> normals;
	vector<uint32_t> tIndices, nIndices;
	if (numTexc) tIndices.resize(m_indices.size());
	if (numNorm) nIndices.resize(m_indices.size());
	normals.reserve(numNorm);

	while (fscanf_s(pFile, "%s", buffer, static_cast<uint32_t>(sizeof(buffer))) != EOF)
	{
		switch (buffer[0])
		{
		case 'f': // v, v//vn, v/vt, or v/vt/vn.
			loadIndices(pFile, numTri, numTexc, numNorm, nIndices, tIndices);
			break;
		case 'v': // v, vn, or vt.
			switch (buffer[1])
			{
			case '\0': // v
			{
				auto& p = getPosition(numVert++);
				fscanf_s(pFile, "%f %f %f", &p.x, &p.y, &p.z);
				if (swapYZ)
				{
					const auto tmp = p.y;
					p.y = p.z;
					p.z = tmp;
				}
				p.z = forDX ? -p.z : p.z;
				break;
			}
			case 'n':
				normals.emplace_back();
				fscanf_s(pFile, "%f %f %f",
					&normals.back().x,
					&normals.back().y,
					&normals.back().z);
				if (swapYZ)
				{
					const auto tmp = normals.back().y;
					normals.back().y = normals.back().z;
					normals.back().z = tmp;
				}
				normals.back().z = forDX ? -normals.back().z : normals.back().z;
			default:
				break;
			}
			break;

		default:
			fgets(buffer, sizeof(buffer), pFile);
			break;
		}
	}

	computePerVertexNormals(normals, nIndices);

	if ((forDX && !swapYZ) || (!forDX && swapYZ)) reverse(m_indices.begin(), m_indices.end());
}

void ObjLoader::loadIndices(FILE* pFile, uint32_t& numTri, uint32_t numTexc,
	uint32_t numNorm, vector<uint32_t>& nIndices, vector<uint32_t>& tIndices)
{
	int64_t vi;
	uint32_t v[3] = { 0 };
	uint32_t vt[3] = { 0 };
	uint32_t vn[3] = { 0 };

	const auto numVert = GetNumVertices();

	for (uint8_t i = 0; i < 3; ++i)
	{
		fscanf_s(pFile, "%lld", &vi);
		v[i] = static_cast<uint32_t>(vi < 0 ? vi + numVert : vi - 1);
		m_indices[numTri * 3 + i] = v[i];

		if (tIndices.size() > 0)
		{
			fscanf_s(pFile, "/%lld", &vi);
			vt[i] = static_cast<uint32_t>(vi < 0 ? vi + numTexc : vi - 1);
			tIndices[numTri * 3 + i] = vt[i];
		}
		else if (nIndices.size() > 0) fscanf_s(pFile, "/");

		if (nIndices.size() > 0)
		{
			fscanf_s(pFile, "/%lld", &vi);
			vn[i] = static_cast<uint32_t>(vi < 0 ? vi + numNorm : vi - 1);
			nIndices[numTri * 3 + i] = vn[i];
		}
	}
	++numTri;

	v[1] = v[2];
	vt[1] = vt[2];
	vn[1] = vn[2];

	while (fscanf_s(pFile, "%lld", &vi) > 0)
	{
		v[2] = static_cast<uint32_t>(vi < 0 ? vi + numVert : vi - 1);
		m_indices[numTri * 3] = v[0];
		m_indices[numTri * 3 + 1] = v[1];
		m_indices[numTri * 3 + 2] = v[2];
		v[1] = v[2];

		if (tIndices.size() > 0)
		{
			fscanf_s(pFile, "/%lld", &vi);
			vt[2] = static_cast<uint32_t>(vi < 0 ? vi + numTexc : vi - 1);
			tIndices[numTri * 3] = vt[0];
			tIndices[numTri * 3 + 1] = vt[1];
			tIndices[numTri * 3 + 2] = vt[2];
			vt[1] = vt[2];
		}
		else if (nIndices.size() > 0) fscanf_s(pFile, "/");

		if (nIndices.size() > 0)
		{
			fscanf_s(pFile, "/%lld", &vi);
			vn[2] = static_cast<uint32_t>(vi < 0 ? vi + numNorm : vi - 1);
			nIndices[numTri * 3] = vn[0];
			nIndices[numTri * 3 + 1] = vn[1];
			nIndices[numTri * 3 + 2] = vn[2];
			vn[1] = vn[2];
		}

		++numTri;
	}
}

void ObjLoader::computePerVertexNormals(const vector<float3>& normals, const vector<uint32_t>& nIndices)
{
	if (normals.empty()) return;

	const auto stride = GetVertexStride();
	vector<uint32_t> vni(GetNumVertices(), UINT32_MAX);

	const auto numIdx = static_cast<uint32_t>(m_indices.size());
	for (auto i = 0u; i < numIdx; i++)
	{
		auto vi = m_indices[i];
		if (vni[vi] == nIndices[i]) continue;

		if (vni[vi] < UINT32_MAX)
		{
			// Split vertex
			vi = GetNumVertices();
			m_vertices.resize(m_vertices.size() + stride);
			const auto pDst = getVertex(vi);
			const auto pSrc = getVertex(m_indices[i]);
			memcpy(pDst, pSrc, stride);
			m_indices[i] = vi;
		}
		else vni[vi] = nIndices[i];

		float3 n = normals[nIndices[i]];
		const auto l = sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
		n.x /= l;
		n.y /= l;
		n.z /= l;

		getNormal(vi) = n;
	}

	m_vertices.shrink_to_fit();
}

void ObjLoader::recomputeNormals()
{
	float3 e1, e2, n;

	const auto numTri = static_cast<uint32_t>(m_indices.size()) / 3;
	for (auto i = 0u; i < numTri; i++)
	{
		const auto pv0 = &getPosition(m_indices[i * 3]);
		const auto pv1 = &getPosition(m_indices[i * 3 + 1]);
		const auto pv2 = &getPosition(m_indices[i * 3 + 2]);
		e1.x = pv1->x - pv0->x;
		e1.y = pv1->y - pv0->y;
		e1.z = pv1->z - pv0->z;
		e2.x = pv2->x - pv1->x;
		e2.y = pv2->y - pv1->y;
		e2.z = pv2->z - pv1->z;
		n.x = e1.y * e2.z - e1.z * e2.y;
		n.y = e1.z * e2.x - e1.x * e2.z;
		n.z = e1.x * e2.y - e1.y * e2.x;
		const auto l = sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
		n.x /= l;
		n.y /= l;
		n.z /= l;

		const auto pVn0 = &getNormal(m_indices[i * 3]);
		const auto pVn1 = &getNormal(m_indices[i * 3 + 1]);
		const auto pVn2 = &getNormal(m_indices[i * 3 + 2]);
		pVn0->x += n.x;
		pVn0->y += n.y;
		pVn0->z += n.z;
		pVn1->x += n.x;
		pVn1->y += n.y;
		pVn1->z += n.z;
		pVn2->x += n.x;
		pVn2->y += n.y;
		pVn2->z += n.z;
	}

	const auto numVert = GetNumVertices();
	for (auto i = 0u; i < numVert; ++i)
	{
		const auto pVn = &getNormal(i);
		const auto l = sqrt(pVn->x * pVn->x + pVn->y * pVn->y + pVn->z * pVn->z);
		pVn->x /= l;
		pVn->y /= l;
		pVn->z /= l;
	}
}

void ObjLoader::computeAABB()
{
	float xMax, xMin, yMax, yMin, zMax, zMin;
	const auto& p = getPosition(0);
	xMax = xMin = p.x;
	yMax = yMin = p.y;
	zMax = zMin = p.z;

	auto x = 0.0f, y = 0.0f, z = 0.0f;

	const auto numVert = GetNumVertices();
	for (auto i = 1u; i < numVert; ++i)
	{
		const auto& p = getPosition(i);
		x = p.x;
		y = p.y;
		z = p.z;

		if (x < xMin) xMin = x;
		else if (x > xMax) xMax = x;

		if (y < yMin) yMin = y;
		else if (y > yMax) yMax = y;

		if (z < zMin) zMin = z;
		else if (z > zMax) zMax = z;
	}

	m_aabb.Min = float3(xMin, yMin, zMin);
	m_aabb.Max = float3(xMax, yMax, zMax);
}

void* ObjLoader::getVertex(uint32_t i)
{
	return &m_vertices[GetVertexStride() * i];
}

ObjLoader::float3& ObjLoader::getPosition(uint32_t i)
{
	return reinterpret_cast<float3*>(getVertex(i))[0];
}

ObjLoader::float3& ObjLoader::getNormal(uint32_t i)
{
	return reinterpret_cast<float3*>(getVertex(i))[1];
}
