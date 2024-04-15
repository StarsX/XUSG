//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGGltfLoader.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "xatlas.h"

using namespace std;
using namespace XUSG;

GltfLoader::GltfLoader()
{
}

GltfLoader::~GltfLoader()
{
}

bool GltfLoader::Import(const char* pszFilename, bool needNorm, bool needColor, bool needAABB, bool invertZ)
{
	m_indices.clear();
	m_lightSources.clear();

	cgltf_options options;
	memset(&options, 0, sizeof(cgltf_options));
	cgltf_data* pData = nullptr;
	cgltf_result result = cgltf_parse_file(&options, pszFilename, &pData);

	if (result == cgltf_result_success)
	{
		result = cgltf_load_buffers(&options, pData, pszFilename);
	}

	if (result == cgltf_result_success)
	{
		result = cgltf_validate(pData);
	}

	if (result == cgltf_result_success)
	{
		string pathDir = pszFilename;
		for (size_t found = pathDir.find('\\'); found != string::npos; found = pathDir.find('\\', found + 1))
			pathDir.replace(found, 1, "/");
		const auto pathDirEnd = pathDir.rfind('/');
		pathDir = pathDir.substr(0, pathDirEnd + 1);

		// Load textures
		m_textures.resize(pData->images_count);
		for (size_t i = 0; i < pData->images_count; ++i)
		{
			const auto texFilePath = pathDir + pData->images[i].uri;
			int width, height, channels;
			const auto infoStat = stbi_info(texFilePath.c_str(), &width, &height, &channels);
			assert(infoStat);
			const auto reqChannels = channels != 3 ? channels : 4;
			const auto pTexData = stbi_load(texFilePath.c_str(), &width, &height, &channels, reqChannels);
			const auto size = sizeof(uint8_t) * reqChannels * width * height;
			m_textures[i].Data.resize(size);
			memcpy(m_textures[i].Data.data(), pTexData, size);
			STBI_FREE(pTexData);
			m_textures[i].Width = width;
			m_textures[i].Height = height;
			m_textures[i].Channels = reqChannels;

			m_texIndexMap[&pData->images[i]] = static_cast<uint32_t>(i);
		}

		m_stride = 0;
		m_posOffset = m_stride;
		m_stride += sizeof(float3);		// position
		m_nrmOffset = m_stride;
		m_stride += sizeof(float3);		// normal
		m_txcOffset = m_stride;
		m_stride += sizeof(float4);		// texcoord
		m_tanOffset = m_stride;
		m_stride += sizeof(float4);		// tangentUV1
		m_colorOffset = m_stride;
		m_stride += sizeof(uint32_t);	// color
		m_scalarOffset = m_stride;
		m_stride += sizeof(float);		// emissiveStrength scalar

		for (auto i = 0u; i < pData->meshes_count; ++i)
		{
			// Load mesh
			const auto& mesh = pData->meshes[i];
			for (auto i = 0u; i < mesh.primitives_count; ++i)
			{
				const auto& primitive = mesh.primitives[i];
				assert(primitive.type == cgltf_primitive_type_triangles);

				// Load vertices
				const cgltf_accessor* pPosition = nullptr;
				const cgltf_accessor* pNormal = nullptr;
				const cgltf_accessor* pTexcoord = nullptr;
				const cgltf_accessor* pColor = nullptr;
				const cgltf_accessor* pTangent = nullptr;
				for (size_t i = 0; i < primitive.attributes_count; ++i)
				{
					const cgltf_attribute_type& type = primitive.attributes[i].type;
					const cgltf_accessor* pAttr = primitive.attributes[i].data;
					assert(pAttr->component_type == cgltf_component_type_r_32f);
					switch (type)
					{
					case cgltf_attribute_type_position:
						assert(pAttr->type == cgltf_type_vec3);
						pPosition = pAttr;
						break;
					case cgltf_attribute_type_normal:
						assert(pAttr->type == cgltf_type_vec3);
						pNormal = pAttr;
						break;
					case cgltf_attribute_type_texcoord:
						assert(pAttr->type == cgltf_type_vec2);
						pTexcoord = pAttr;
						break;
					case cgltf_attribute_type_color:
						assert(pAttr->type == cgltf_type_vec3);
						pColor = pAttr;
						break;
					case cgltf_attribute_type_tangent:
						assert(pAttr->type == cgltf_type_vec4);
						pTangent = pAttr;
						break;
					}
				}

				const uint32_t vertexOffset = static_cast<uint32_t>(m_vertices.size() / m_stride);
				const uint32_t vertexBufferOffset = static_cast<uint32_t>(m_vertices.size());
				const uint32_t vertexCount = pPosition ? static_cast<uint32_t>(pPosition->count) : 0;
				m_vertices.resize(vertexBufferOffset + vertexCount * m_stride);
				{
					assert(pPosition);
					for (uint32_t i = 0; i < pPosition->count; ++i)
					{
						float3 p;
						cgltf_accessor_read_float(pPosition, i, &p.x, cgltf_num_components(cgltf_type_vec3));
						p.z = invertZ ? -p.z : p.z;
						getPosition(vertexOffset + i) = p;
					}
				}

				if (pNormal)
				{
					assert(vertexCount == pNormal->count);
					for (uint32_t i = 0; i < pNormal->count; ++i)
					{
						float3 n;
						cgltf_accessor_read_float(pNormal, i, &n.x, cgltf_num_components(cgltf_type_vec3));
						n.z = invertZ ? -n.z : n.z;
						getNormal(vertexOffset + i) = n;
					}
				}

				if (pNormal)
				{
					assert(vertexCount == pTexcoord->count);
					for (uint32_t i = 0; i < pTexcoord->count; ++i)
					{
						float2 t;
						cgltf_accessor_read_float(pTexcoord, i, &t.x, cgltf_num_components(cgltf_type_vec2));
						getTexcoord(vertexOffset + i) = float4(t.x, t.y, t.x, t.y);
					}
				}

				if (pTangent)
				{
					assert(vertexCount == pTangent->count);
					for (uint32_t i = 0; i < pTangent->count; ++i)
					{
						float4 t;
						cgltf_accessor_read_float(pTangent, i, &t.x, cgltf_num_components(cgltf_type_vec4));
						t.z = invertZ ? -t.z : t.z;
						t.w = invertZ ? -t.w : t.w;
						getTangent(vertexOffset + i) = t;
					}
				}
				else for (uint32_t i = 0; i < vertexCount; ++i) getTangent(vertexOffset + i) = float4(0.0f);

				if (pColor)
				{
					assert(vertexCount == pColor->count);
					for (uint32_t i = 0; i < pColor->count; ++i)
					{
						float3 c;
						cgltf_accessor_read_float(pColor, i, &c.x, cgltf_num_components(cgltf_type_vec3));
						getVertexColor(vertexOffset + i) = float4_to_rgba8(float4(c.x, c.y, c.z, 1.0f));
					}
				}

				// Load indices
				const cgltf_accessor* pIndices = primitive.indices;
				if (pData->textures_count)
				{
					m_subsets.emplace_back();
					auto& subset = m_subsets.back();
					subset.IndexOffset = static_cast<uint32_t>(m_indices.size());
					subset.NumIndices = static_cast<uint32_t>(pIndices->count);
					subset.BaseColorTexIdx = UINT32_MAX;
					subset.NormalTexIdx = UINT32_MAX;
					subset.MtlRghTexIdx = UINT32_MAX;
				}
				m_indices.reserve(m_indices.size() + pIndices->count);
				for (size_t i = 0; i < pIndices->count; ++i)
				{
					assert(pIndices->component_type == cgltf_component_type_r_16u || pIndices->component_type == cgltf_component_type_r_32u);
					const auto index = cgltf_accessor_read_index(pIndices, i);
					m_indices.emplace_back(vertexOffset + static_cast<uint32_t>(index));
				}

				// Material and textures
				if (pData->textures_count && primitive.material)
				{
					auto& subset = m_subsets.back();
					assert(primitive.material->has_pbr_metallic_roughness);
					if (primitive.material->pbr_metallic_roughness.base_color_texture.texture)
					{
						assert(primitive.material->pbr_metallic_roughness.base_color_texture.texture);
						assert(primitive.material->pbr_metallic_roughness.base_color_texture.texture->image);
						const auto iter = m_texIndexMap.find(primitive.material->pbr_metallic_roughness.base_color_texture.texture->image);
						assert(iter != m_texIndexMap.cend());
						subset.BaseColorTexIdx = iter->second;
					}
					if (primitive.material->pbr_metallic_roughness.metallic_roughness_texture.texture)
					{
						assert(primitive.material->pbr_metallic_roughness.metallic_roughness_texture.texture);
						assert(primitive.material->pbr_metallic_roughness.metallic_roughness_texture.texture->image);
						const auto iter = m_texIndexMap.find(primitive.material->pbr_metallic_roughness.metallic_roughness_texture.texture->image);
						assert(iter != m_texIndexMap.cend());
						subset.MtlRghTexIdx = iter->second;
					}
					if (primitive.material->normal_texture.texture)
					{
						assert(primitive.material->normal_texture.texture);
						assert(primitive.material->normal_texture.texture->image);
						const auto iter = m_texIndexMap.find(primitive.material->normal_texture.texture->image);
						assert(iter != m_texIndexMap.cend());
						subset.NormalTexIdx = iter->second;
					}
					subset.AlphaMode = static_cast<AlphaMode>(primitive.material->alpha_mode);
				}

				// Perform post import tasks.
				if (needColor && !pColor)
				{
					float4 vertexColor(1.0f, 1.0f, 1.0f, 1.0f);
					float vertexScalar = 0.0f;
					if (primitive.material)
					{
						if (primitive.material->has_emissive_strength)
						{
							float emissiveStrength = primitive.material->emissive_strength.emissive_strength;
							float* emissiveFactor = primitive.material->emissive_factor;
							vertexColor.x = emissiveFactor[0];
							vertexColor.y = emissiveFactor[1];
							vertexColor.z = emissiveFactor[2];
							vertexScalar = emissiveStrength;

							m_lightSources.push_back({ float4(FLT_MAX, 1.0f), float4(-FLT_MAX, 1.0f) });
							auto& lightSource = m_lightSources.back();
							for (uint32_t i = 0; i < pPosition->count; ++i)
							{
								float3 p;
								cgltf_accessor_read_float(pPosition, i, &p.x, cgltf_num_components(cgltf_type_vec3));
								p.z = invertZ ? -p.z : p.z;

								lightSource.Min.x = (min)(p.x, lightSource.Min.x);
								lightSource.Min.y = (min)(p.y, lightSource.Min.y);
								lightSource.Min.z = (min)(p.z, lightSource.Min.z);
								lightSource.Max.x = (max)(p.x, lightSource.Max.x);
								lightSource.Max.y = (max)(p.y, lightSource.Max.y);
								lightSource.Max.z = (max)(p.z, lightSource.Max.z);
							}

							lightSource.Emissive = vertexColor;
							lightSource.Emissive.w = vertexScalar;
						}
						else if (primitive.material->has_pbr_metallic_roughness)
						{
							float* baseColor = primitive.material->pbr_metallic_roughness.base_color_factor;
							vertexColor.x = baseColor[0];
							vertexColor.y = baseColor[1];
							vertexColor.z = baseColor[2];
						}
					}

					fillVertexColors(vertexOffset, vertexCount, vertexColor);
					fillVertexScalars(vertexOffset, vertexCount, vertexScalar);
				}
				if (needNorm && !pNormal) recomputeNormals();
				if (needAABB) computeAABB();

				// Light-map texcoord/procedural texcoord
				if (pData->textures_count || !pTexcoord) regenerateUV1(vertexOffset, vertexCount, pTexcoord);
			}
		}

		if (!pData->textures_count)
		{
			// If no image/texture, merge all meshes into one subset
			m_subsets.emplace_back();
			auto& subset = m_subsets.back();
			subset.IndexOffset = 0;
			subset.NumIndices = GetNumIndices();
			subset.BaseColorTexIdx = UINT32_MAX;
			subset.NormalTexIdx = UINT32_MAX;
			subset.MtlRghTexIdx = UINT32_MAX;
			subset.LightMapScl = float2(1.0f);
		}

		// Adjust index windings
		if (invertZ)
		{
			for (size_t i = 0; i < m_indices.size() / 3; ++i)
			{
				const auto tmp = m_indices[i * 3];
				m_indices[i * 3] = m_indices[i * 3 + 2];
				m_indices[i * 3 + 2] = tmp;
			}
		}
	}

	cgltf_free(pData);

	return result == cgltf_result_success;
}

const uint32_t GltfLoader::GetNumVertices() const
{
	return static_cast<uint32_t>(m_vertices.size() / GetVertexStride());
}

const uint32_t GltfLoader::GetNumIndices() const
{
	return static_cast<uint32_t>(m_indices.size());
}

const uint32_t GltfLoader::GetNumSubSets() const
{
	return static_cast<uint32_t>(m_subsets.size());
}

const uint32_t GltfLoader::GetNumTextures() const
{
	return static_cast<uint32_t>(m_textures.size());
}

const uint32_t GltfLoader::GetVertexStride() const
{
	return m_stride;
}

const uint8_t* GltfLoader::GetVertices() const
{
	return m_vertices.data();
}

const uint32_t* GltfLoader::GetIndices() const
{
	return m_indices.data();
}

const GltfLoader::Subset* GltfLoader::GetSubsets() const
{
	return m_subsets.data();
}

const GltfLoader::Texture* GltfLoader::GetTextures() const
{
	return m_textures.data();
}

const GltfLoader::AABB& GltfLoader::GetAABB() const
{
	return m_aabb;
}

const vector<GltfLoader::LightSource>& GltfLoader::GetLightSources() const
{
	return m_lightSources;
}

void GltfLoader::fillVertexColors(uint32_t offset, uint32_t size, float4 color)
{
	for (auto i = 0u; i < size; ++i)
	{
		getVertexColor(offset + i) = float4_to_rgba8(color);
	}
}

void GltfLoader::fillVertexScalars(uint32_t offset, uint32_t size, float scalar)
{
	for (auto i = 0u; i < size; ++i)
	{
		getVertexScalar(offset + i) = scalar;
	}
}

void GltfLoader::recomputeNormals()
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

void GltfLoader::computeAABB()
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

void GltfLoader::regenerateUV1(uint32_t vertexOffset, uint32_t vertexCount, bool useInputMeshUvs)
{
	auto& subset = m_subsets.back();

	xatlas::MeshDecl meshDecl;
	meshDecl.vertexPositionData = &getPosition(vertexOffset);
	meshDecl.vertexNormalData = &getNormal(vertexOffset);
	meshDecl.vertexUvData = &getTexcoord(vertexOffset); // optional. The input UVs are provided as a hint to the chart generator.
	meshDecl.indexData = &m_indices[subset.IndexOffset];

	// Optional. Must be faceCount in length.
	// Don't atlas faces set to true. Ignored faces still exist in the output meshes, Vertex uv is set to (0, 0) and Vertex atlasIndex to -1.
	//meshDecl.faceIgnoreData = nullptr;

	// Optional. Must be faceCount in length.
	// Only faces with the same material will be assigned to the same chart.
	//meshDecl.faceMaterialData = nullptr;

	// Optional. Must be faceCount in length.
	// Polygon / n-gon support. Faces are assumed to be triangles if this is null.
	//meshDecl.faceVertexCount = nullptr;

	meshDecl.vertexCount = vertexCount;
	meshDecl.vertexPositionStride = m_stride;
	meshDecl.vertexNormalStride = m_stride;
	meshDecl.vertexUvStride = m_stride; // optional
	meshDecl.indexCount = subset.NumIndices;
	meshDecl.indexOffset = -static_cast<int32_t>(vertexOffset); // optional. Add this offset to all indices.
	//meshDecl.faceCount = 0; // Optional if faceVertexCount is null. Otherwise assumed to be indexCount / 3.
	meshDecl.indexFormat = xatlas::IndexFormat::UInt32;

	xatlas::Atlas* pAtlas = xatlas::Create();
	xatlas::AddMeshError error = xatlas::AddMesh(pAtlas, meshDecl);
	if (error != xatlas::AddMeshError::Success)
	{
		subset.LightMapScl = float2(1.0f);
		xatlas::Destroy(pAtlas);
		return;
	}

	const auto& texture = m_textures[subset.BaseColorTexIdx];
	xatlas::ChartOptions chartOptions;
	//chartOptions.maxIterations = 4;
	chartOptions.useInputMeshUvs = useInputMeshUvs;
	//chartOptions.fixWinding = true;
	xatlas::PackOptions packOptions;
	//packOptions.padding = 1;
	//packOptions.bruteForce = true;
	packOptions.blockAlign = true;
	packOptions.resolution = (max)(texture.Width, texture.Height);
	xatlas::Generate(pAtlas, chartOptions);

	assert(pAtlas->atlasCount == 1);
	assert(pAtlas->meshCount == 1);
	assert(pAtlas->meshes[0].indexCount == subset.NumIndices);
	vector<uint8_t> vertices(m_stride * vertexCount);
	memcpy(vertices.data(), &m_vertices[m_stride * vertexOffset], vertices.size());
	vertexCount = pAtlas->meshes[0].vertexCount;
	m_vertices.resize(m_stride * (vertexOffset + vertexCount));
	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		const auto& vertex = pAtlas->meshes[0].vertexArray[i];
		memcpy(getVertex(vertexOffset + i), &vertices[m_stride * vertex.xref], m_stride);
		auto& uv = getTexcoord(vertexOffset + i);
		uv.z = vertex.uv[0] / static_cast<float>(pAtlas->width);
		uv.w = vertex.uv[1] / static_cast<float>(pAtlas->height);
		if (!useInputMeshUvs)
		{
			uv.x = uv.z;
			uv.y = uv.w;
		}
	}

	for (uint32_t i = 0; i < subset.NumIndices; ++i)
		m_indices[subset.IndexOffset + i] = vertexOffset + pAtlas->meshes[0].indexArray[i];

	subset.LightMapScl.x = pAtlas->width / static_cast<float>(texture.Width);
	subset.LightMapScl.y = pAtlas->height / static_cast<float>(texture.Height);
	subset.LightMapScl.x = exp2f(roundf(log2f(subset.LightMapScl.x)));
	subset.LightMapScl.y = exp2f(roundf(log2f(subset.LightMapScl.y)));

	xatlas::Destroy(pAtlas);
}

uint8_t* GltfLoader::getVertex(uint32_t i)
{
	return &m_vertices[GetVertexStride() * i];
}

GltfLoader::float3& GltfLoader::getPosition(uint32_t i)
{
	return reinterpret_cast<float3&>(getVertex(i)[m_posOffset]);
}

GltfLoader::float3& GltfLoader::getNormal(uint32_t i)
{
	return reinterpret_cast<float3&>(getVertex(i)[m_nrmOffset]);
}

GltfLoader::float4& GltfLoader::getTexcoord(uint32_t i)
{
	return reinterpret_cast<float4&>(getVertex(i)[m_txcOffset]);
}

GltfLoader::float4& GltfLoader::getTangent(uint32_t i)
{
	return reinterpret_cast<float4&>(getVertex(i)[m_tanOffset]);
}

uint32_t& GltfLoader::getVertexColor(uint32_t i)
{
	return reinterpret_cast<uint32_t&>(getVertex(i)[m_colorOffset]);
}

float& GltfLoader::getVertexScalar(uint32_t i)
{
	return reinterpret_cast<float&>(getVertex(i)[m_scalarOffset]);
}

uint32_t GltfLoader::float4_to_rgba8(const float4& f)
{
	uint32_t packedOutput;
	packedOutput = ((float_to_uint(saturateFloat(f.x), 255.0f)) |
		(float_to_uint(saturateFloat(f.y), 255.0f) << 8) |
		(float_to_uint(saturateFloat(f.z), 255.0f) << 16) |
		(float_to_uint(saturateFloat(f.w), 255.0f) << 24));

	return packedOutput;
}

float GltfLoader::saturateFloat(float f)
{
	return (min)((max)(f, 0.0f), 1.0f);
}

uint32_t GltfLoader::float_to_uint(float f, float scale)
{
	return static_cast<uint32_t>(floorf(f * scale + 0.5f));
}
