//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGGltfLoader.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

using namespace std;
using namespace XUSG;

GltfLoader::GltfLoader()
{
}

GltfLoader::~GltfLoader()
{
}

bool GltfLoader::Import(const char* pszFilename, bool needNorm, bool needColor, bool needAABB, bool forDX)
{
	m_lightSources.clear();

	cgltf_options options;
	memset(&options, 0, sizeof(cgltf_options));
	cgltf_data* data = nullptr;
	cgltf_result result = cgltf_parse_file(&options, pszFilename, &data);

	if (result == cgltf_result_success)
	{
		result = cgltf_load_buffers(&options, data, pszFilename);
	}

	if (result == cgltf_result_success)
	{
		result = cgltf_validate(data);
	}

	if (result == cgltf_result_success)
	{
		for (auto i = 0u; i < data->meshes_count; ++i)
		{
			// load mesh.
			const auto& mesh = data->meshes[i];
			const auto& primitive = mesh.primitives[0];
			assert(primitive.type == cgltf_primitive_type_triangles);

			// load vertices.
			auto hasNormalData = false;
			auto hasVertexColorData = false;
			const cgltf_accessor* position = nullptr;
			const cgltf_accessor* normal = nullptr;
			const cgltf_accessor* texcoord = nullptr;
			const cgltf_accessor* color = nullptr;
			for (size_t i = 0; i < primitive.attributes_count; ++i)
			{
				const cgltf_attribute_type type = primitive.attributes[i].type;
				const cgltf_accessor* attr = primitive.attributes[i].data;
				assert(attr->component_type == cgltf_component_type_r_32f);
				if (cgltf_attribute_type_position == type)
				{
					assert(attr->type == cgltf_type_vec3);
					position = attr;
				}
				else if (cgltf_attribute_type_normal == type)
				{
					assert(attr->type == cgltf_type_vec3);
					hasNormalData = true;
					normal = attr;
				}
				else if (cgltf_attribute_type_texcoord == type)
				{
					assert(attr->type == cgltf_type_vec2);
					texcoord = attr;
				}
				else if (cgltf_attribute_type_color == type)
				{
					assert(attr->type == cgltf_type_vec3);
					hasVertexColorData = true;
					color = attr;
				}
			}

			m_stride = 0;
			m_posOffset = m_stride;
			m_stride += sizeof(float3); // position
			m_nrmOffset = m_stride;
			m_stride += sizeof(float3); // normal
			m_txcOffset = m_stride;
			m_stride += sizeof(float2); // texcoord
			m_colorOffset = m_stride;
			m_stride += sizeof(uint32_t); // color
			m_scalarOffset = m_stride;
			m_stride += sizeof(float); // emissiveStrength scalar

			const uint32_t vertexOffset = static_cast<uint32_t>(m_vertices.size() / m_stride);
			const uint32_t vertexBufferOffset = static_cast<uint32_t>(m_vertices.size());
			const uint32_t vertexCount = static_cast<uint32_t>(position->count);
			m_vertices.resize(vertexBufferOffset + vertexCount * m_stride);

			for (uint32_t i = 0; position && i < position->count; ++i)
			{
				char* data = (char*)position->buffer_view->buffer->data + position->buffer_view->offset;
				const size_t stride = position->stride;
				float3* const p = reinterpret_cast<float3*>(data + i * stride);
				p->z = forDX ? -p->z : p->z;
				getPosition(vertexOffset + i) = *p;
			}

			for (uint32_t i = 0; normal && i < normal->count; ++i)
			{
				char* data = (char*)normal->buffer_view->buffer->data + normal->buffer_view->offset;
				const size_t stride = normal->stride;
				float3* const n = reinterpret_cast<float3*>(data + i * stride);
				n->z = forDX ? -n->z : n->z;
				getNormal(vertexOffset + i) = *n;
			}

			for (uint32_t i = 0; texcoord && i < texcoord->count; ++i)
			{
				char* data = (char*)texcoord->buffer_view->buffer->data + texcoord->buffer_view->offset;
				const size_t stride = texcoord->stride;
				float2* const t = reinterpret_cast<float2*>(data + i * stride);
				getTexcoord(vertexOffset + i) = *t;
			}

			for (uint32_t i = 0; color && i < color->count; ++i)
			{
				char* data = (char*)color->buffer_view->buffer->data + color->buffer_view->offset;
				const size_t stride = color->stride;
				float3* const c = reinterpret_cast<float3*>(data + i * stride);
				getVertexColor(vertexOffset + i) = float4_to_rgba8(float4(c->x, c->y, c->z, 1.0f));
			}

			// load indices.
			const cgltf_accessor* indices = primitive.indices;
			for (size_t i = 0; i < indices->count; ++i)
			{
				const cgltf_component_type component_type = indices->component_type;
				assert(component_type == cgltf_component_type_r_16u
					|| component_type == cgltf_component_type_r_32u);

				if (component_type == cgltf_component_type_r_16u)
				{
					m_indices.reserve(indices->count);
					char* data = (char*)indices->buffer_view->buffer->data + indices->buffer_view->offset;
					uint16_t index = *reinterpret_cast<uint16_t*>(data + i * indices->stride);
					m_indices.push_back(vertexOffset + index);
				}
				else if (component_type == cgltf_component_type_r_32u)
				{
					m_indices.reserve(indices->count);
					char* data = (char*)indices->buffer_view->buffer->data + indices->buffer_view->offset;
					uint32_t index = *reinterpret_cast<uint32_t*>(data + i * indices->stride);
					m_indices.push_back(vertexOffset + index);
				}
			}

			// Perform post import tasks.
			if (needColor && !hasVertexColorData)
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
						for (uint32_t i = 0; position && i < position->count; ++i)
						{
							char* data = (char*)position->buffer_view->buffer->data + position->buffer_view->offset;
							const size_t stride = position->stride;
							float3* const p = reinterpret_cast<float3*>(data + i * stride);

							lightSource.Min.x = (min)(p->x, lightSource.Min.x);
							lightSource.Min.y = (min)(p->y, lightSource.Min.y);
							lightSource.Min.z = (min)(p->z, lightSource.Min.z);
							lightSource.Max.x = (max)(p->x, lightSource.Max.x);
							lightSource.Max.y = (max)(p->y, lightSource.Max.y);
							lightSource.Max.z = (max)(p->z, lightSource.Max.z);
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
			if (needNorm && !hasNormalData) recomputeNormals();
			if (needAABB) computeAABB();
		}

		// Adjust index windings
		if (forDX)
		{
			for (size_t i = 0; i < m_indices.size() / 3; ++i)
			{
				const auto tmp = m_indices[i * 3];
				m_indices[i * 3] = m_indices[i * 3 + 2];
				m_indices[i * 3 + 2] = tmp;
			}
		}
	}

	cgltf_free(data);
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
	for (auto i = 0u; i < size; i++)
	{
		getVertexColor(offset + i) = float4_to_rgba8(color);
	}
}

void GltfLoader::fillVertexScalars(uint32_t offset, uint32_t size, float scalar)
{
	for (auto i = 0u; i < size; i++)
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

GltfLoader::float2& GltfLoader::getTexcoord(uint32_t i)
{
	return reinterpret_cast<float2&>(getVertex(i)[m_txcOffset]);
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
