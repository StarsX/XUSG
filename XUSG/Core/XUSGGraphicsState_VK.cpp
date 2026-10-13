//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGGraphicsState_VK.h"
#include "XUSGGraphicsState_DX12.h"
#include "XUSGEnum_VK.h"

using namespace std;
using namespace XUSG;
using namespace Graphics;

State_VK::State_VK()
{
	// Default state
	m_key.resize(sizeof(PipelineDesc));
	m_pKey = reinterpret_cast<PipelineDesc*>(&m_key[0]);
	memset(m_pKey, 0, sizeof(PipelineDesc));
	m_pKey->SampleMask = UINT_MAX;
	m_pKey->PrimTopologyType = PrimitiveTopologyType::TRIANGLE;
	m_pKey->SampleCount = 1;
}

State_VK::~State_VK()
{
}

void* State_VK::GetBackend()
{
	return this;
}

void State_VK::SetPipelineLayout(const PipelineLayout& layout)
{
	m_pKey->Layout = layout;
}

void State_VK::SetShader(Shader::Stage stage, const Blob& shader)
{
	m_pKey->Shaders[stage] = shader;
}

void State_VK::SetCachedPipeline(const Blob& cachedPipeline)
{
	m_pKey->CachedPipeline = cachedPipeline;
}

void State_VK::SetNodeMask(uint32_t nodeMask)
{
	m_pKey->NodeMask = nodeMask;
}

void State_VK::SetFlags(PipelineFlag flag)
{
	m_pKey->Flags = flag;
}

void State_VK::OMSetBlendState(const Blend* pBlend, uint32_t sampleMask)
{
	m_pKey->pBlend = pBlend;
	m_pKey->SampleMask = sampleMask;
}

void State_VK::RSSetState(const Rasterizer* pRasterizer)
{
	m_pKey->pRasterizer = pRasterizer;
}

void State_VK::DSSetState(const DepthStencil* pDepthStencil)
{
	m_pKey->pDepthStencil = pDepthStencil;
}

void State_VK::OMSetBlendState(BlendPreset preset, PipelineLib* pPipelineLib, uint8_t numColorRTs, uint32_t sampleMask)
{
	OMSetBlendState(pPipelineLib->GetBlend(preset, numColorRTs), sampleMask);
}

void State_VK::RSSetState(RasterizerPreset preset, PipelineLib* pPipelineLib)
{
	RSSetState(pPipelineLib->GetRasterizer(preset));
}

void State_VK::DSSetState(DepthStencilPreset preset, PipelineLib* pPipelineLib)
{
	DSSetState(pPipelineLib->GetDepthStencil(preset));
}

void State_VK::IASetInputLayout(const InputLayout* pLayout)
{
	m_pKey->pInputLayout = pLayout;
}

void State_VK::IASetPrimitiveTopologyType(PrimitiveTopologyType type)
{
	m_pKey->PrimTopologyType = type;
}

void State_VK::IASetIndexBufferStripCutValue(IBStripCutValue ibStripCutValue)
{
	m_pKey->IBStripCutValue = static_cast<uint8_t>(ibStripCutValue);
}

void State_VK::OMSetNumRenderTargets(uint8_t n)
{
	const auto numRTVFormats = static_cast<uint8_t>(size(m_pKey->RTVFormats));
	assert(n <= numRTVFormats);
	m_pKey->NumRenderTargets = n;

	for (auto i = n; i < numRTVFormats; ++i) OMSetRTVFormat(i, Format::UNKNOWN);
}

void State_VK::OMSetRTVFormat(uint8_t i, Format format)
{
	assert(i < size(m_pKey->RTVFormats));
	m_pKey->RTVFormats[i] = format;
}

void State_VK::OMSetRTVFormats(uint8_t n, const Format* formats)
{
	OMSetNumRenderTargets(n);

	for (uint8_t i = 0; i < n; ++i) OMSetRTVFormat(i, formats[i]);
}

void State_VK::OMSetDSVFormat(Format format)
{
	m_pKey->DSVFormat = format;
}

void State_VK::OMSetSample(uint8_t count, uint8_t quality)
{
	m_pKey->SampleCount = count;
	m_pKey->SampleQuality = quality;
}

Pipeline State_VK::CreatePipeline(PipelineLib* pPipelineLib, const wchar_t* name) const
{
	return pPipelineLib->CreatePipeline(this, name);
}

Pipeline State_VK::GetPipeline(PipelineLib* pPipelineLib, const wchar_t* name) const
{
	return pPipelineLib->GetPipeline(this, name);
}

PipelineLayout State_VK::GetPipelineLayout() const
{
	return m_pKey->Layout;
}

Blob State_VK::GetShader(Shader::Stage stage) const
{
	assert(stage < Shader::Stage::NUM_GRAPHICS);

	return m_pKey->Shaders[stage];
}

Blob State_VK::GetCachedPipeline() const
{
	return m_pKey->CachedPipeline;
}

uint32_t State_VK::GetNodeMask() const
{
	return m_pKey->NodeMask;
}

PipelineFlag State_VK::GetFlags() const
{
	return m_pKey->Flags;
}

uint32_t State_VK::OMGetSampleMask() const
{
	return m_pKey->SampleMask;
}

const Graphics::Blend* State_VK::OMGetBlendState() const
{
	return m_pKey->pBlend;
}

const Graphics::Rasterizer* State_VK::RSGetState() const
{
	return m_pKey->pRasterizer;
}

const Graphics::DepthStencil* State_VK::DSGetState() const
{
	return m_pKey->pDepthStencil;
}

const InputLayout* State_VK::IAGetInputLayout() const
{
	return m_pKey->pInputLayout;
}

PrimitiveTopologyType State_VK::IAGetPrimitiveTopologyType() const
{
	return m_pKey->PrimTopologyType;
}

IBStripCutValue State_VK::IAGetIndexBufferStripCutValue() const
{
	return static_cast<IBStripCutValue>(m_pKey->IBStripCutValue);
}

uint8_t State_VK::OMGetNumRenderTargets() const
{
	return m_pKey->NumRenderTargets;
}

Format State_VK::OMGetRTVFormat(uint8_t i) const
{
	assert(i < m_pKey->NumRenderTargets);

	return m_pKey->RTVFormats[i];
}

Format State_VK::OMGetDSVFormat() const
{
	return m_pKey->DSVFormat;
}

uint8_t State_VK::OMGetSampleCount() const
{
	return m_pKey->SampleCount;
}

uint8_t State_VK::OMGetSampleQuality() const
{
	return m_pKey->SampleQuality;
}

void State_VK::GetHandleDesc(void* pHandleDesc, void* pInputElements) const
{
	(void)pHandleDesc;
	(void)pInputElements;
}

const string& State_VK::GetKey() const
{
	return m_key;
}

//--------------------------------------------------------------------------------------

PipelineLib_VK::PipelineLib_VK() :
	m_pDevice(nullptr)
{
	InitGraphicsPresets(m_pfnBlends, m_pfnRasterizers, m_pfnDepthStencils);
}

PipelineLib_VK::PipelineLib_VK(Device* pDevice) :
	PipelineLib_VK()
{
	SetDevice(pDevice);
}

PipelineLib_VK::~PipelineLib_VK()
{
	if (m_pipelines.empty()) return;
	const auto device = m_pDevice->GetDevice();
	for (auto& pipeline : m_pipelines) ReleaseVKPipeline(device, pipeline.second);
}

void PipelineLib_VK::SetDevice(Device* pDevice)
{
	m_pDevice = AsVK(pDevice);
	assert(m_pDevice);
}

void PipelineLib_VK::SetPipeline(const State* pState, const Pipeline& pipeline)
{
	lock_guard<mutex> lock(m_mtx);
	const auto p = AsVK(pState);
	assert(p);
	const auto& key = p->GetKey();
	const auto incoming = static_cast<Pipeline_VK*>(pipeline);
	RetainVKPipeline(incoming);
	const auto existing = m_pipelines.find(key);
	if (existing != m_pipelines.end()) ReleaseVKPipeline(m_pDevice->GetDevice(), existing->second);
	m_pipelines[key] = incoming;
}

void PipelineLib_VK::SetInputLayout(uint32_t index, uint32_t numElements, const InputElement* pElements)
{
	m_inputLayoutLib.SetLayout(index, numElements, pElements);
}

const InputLayout* PipelineLib_VK::GetInputLayout(uint32_t index) const
{
	return m_inputLayoutLib.GetLayout(index);
}

const InputLayout* PipelineLib_VK::CreateInputLayout(uint32_t numElements, const InputElement* pElements)
{
	return m_inputLayoutLib.CreateLayout(numElements, pElements);
}

Pipeline PipelineLib_VK::CreatePipeline(const State* pState, const wchar_t* name)
{
	const auto p = AsVK(pState);
	assert(p);
	lock_guard<mutex> lock(m_mtx);

	return createPipeline(p->GetKey(), name);
}

Pipeline PipelineLib_VK::GetPipeline(const State* pState, const wchar_t* name)
{
	const auto p = AsVK(pState);
	assert(p);
	lock_guard<mutex> lock(m_mtx);

	return getPipeline(p->GetKey(), name);
}

const Blend* PipelineLib_VK::GetBlend(BlendPreset preset, uint8_t numColorRTs)
{
	if (!m_blends[preset]) m_blends[preset] = make_unique<Blend>(m_pfnBlends[preset](numColorRTs));

	return m_blends[preset].get();
}

const Rasterizer* PipelineLib_VK::GetRasterizer(RasterizerPreset preset)
{
	if (!m_rasterizers[preset]) m_rasterizers[preset] = make_unique<Rasterizer>(m_pfnRasterizers[preset]());

	return m_rasterizers[preset].get();
}

const Graphics::DepthStencil* PipelineLib_VK::GetDepthStencil(DepthStencilPreset preset)
{
	if (!m_depthStencils[preset]) m_depthStencils[preset] = make_unique<Graphics::DepthStencil>(m_pfnDepthStencils[preset]());

	return m_depthStencils[preset].get();
}

namespace
{
	VkShaderModule CreateShaderModule(VkDevice device, const Blob& blob)
	{
		const void* pData = nullptr;
		size_t size = 0;
		GetVKBlobData(blob, &pData, &size);
		if (!pData || !size) return VK_NULL_HANDLE;

		VkShaderModuleCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.codeSize = size;
		info.pCode = static_cast<const uint32_t*>(pData);
		VkShaderModule module = VK_NULL_HANDLE;
		if (vkCreateShaderModule(device, &info, nullptr, &module) < 0) return VK_NULL_HANDLE;

		return module;
	}

	bool SpirvNameEquals(const uint32_t* words, uint32_t wordCount, const char* name)
	{
		auto n = 0u;
		for (auto w = 0u; w < wordCount; ++w)
		{
			const auto word = words[w];
			for (auto b = 0u; b < 4u; ++b)
			{
				const auto c = static_cast<char>((word >> (b * 8)) & 0xff);
				if (c == '\0') return name[n] == '\0';
				if (name[n] == '\0' || c != name[n]) return false;
				++n;
			}
		}

		return false;
	}

	constexpr uint32_t SpirvMagic = 0x07230203;
	constexpr uint32_t MaxPatchControlPoints = 32;

	uint32_t SpirvInputControlPointCount(const Blob& blob, const char* entry)
	{
		constexpr uint32_t OpEntryPoint = 15;
		constexpr uint32_t OpTypeArray = 28;
		constexpr uint32_t OpTypePointer = 32;
		constexpr uint32_t OpConstant = 43;
		constexpr uint32_t OpVariable = 59;
		constexpr uint32_t ExecutionModelTessellationControl = 3;
		constexpr uint32_t StorageClassInput = 1;

		const void* pData = nullptr;
		size_t size = 0;
		GetVKBlobData(blob, &pData, &size);
		if (!pData || size < 20 || size % 4 || !entry) return 0;

		const auto words = static_cast<const uint32_t*>(pData);
		const auto count = size / sizeof(uint32_t);
		if (words[0] != SpirvMagic) return 0;

		unordered_map<uint32_t, uint32_t> constants;
		unordered_map<uint32_t, uint32_t> arrayLengths;
		unordered_map<uint32_t, uint32_t> pointerTypes;
		unordered_map<uint32_t, pair<uint32_t, uint32_t>> variables;
		vector<uint32_t> interfaces;
		auto matchedEntry = false;
		for (auto i = 5u; i < count;)
		{
			const auto wordCount = words[i] >> 16;
			const auto opcode = words[i] & 0xffff;
			if (!wordCount || i + wordCount > count) break;

			if (opcode == OpConstant && wordCount >= 4)
				constants[words[i + 2]] = words[i + 3];
			else if (opcode == OpTypeArray && wordCount >= 4)
				arrayLengths[words[i + 1]] = words[i + 3];
			else if (opcode == OpTypePointer && wordCount >= 4 && words[i + 2] == StorageClassInput)
				pointerTypes[words[i + 1]] = words[i + 3];
			else if (opcode == OpVariable && wordCount >= 4)
				variables[words[i + 2]] = { words[i + 3], words[i + 1] };
			else if (opcode == OpEntryPoint && wordCount >= 4 &&
				words[i + 1] == ExecutionModelTessellationControl &&
				SpirvNameEquals(&words[i + 3], wordCount - 3, entry))
			{
				auto nameWords = 0u;
				for (; nameWords < wordCount - 3; ++nameWords)
				{
					const auto word = words[i + 3 + nameWords];
					if (!(word & 0xff) || !(word & 0xff00) || !(word & 0xff0000) || !(word & 0xff000000))
					{
						++nameWords;
						break;
					}
				}
				interfaces.clear();
				for (auto w = 3u + nameWords; w < wordCount; ++w) interfaces.push_back(words[i + w]);
				matchedEntry = true;
			}

			i += wordCount;
		}
		if (!matchedEntry)
		{
			interfaces.clear();
			for (const auto& variable : variables)
				if (variable.second.first == StorageClassInput) interfaces.push_back(variable.first);
		}

		auto patchCount = 0u;
		for (const auto id : interfaces)
		{
			const auto variable = variables.find(id);
			if (variable == variables.end() || variable->second.first != StorageClassInput) continue;
			const auto pointee = pointerTypes.find(variable->second.second);
			if (pointee == pointerTypes.end()) continue;
			const auto lengthId = arrayLengths.find(pointee->second);
			if (lengthId == arrayLengths.end()) continue;
			const auto value = constants.find(lengthId->second);
			if (value == constants.end() || !value->second || value->second > MaxPatchControlPoints) continue;
			assert(!patchCount || patchCount == value->second);
			if (!patchCount) patchCount = value->second;
		}

		return patchCount;
	}

	uint32_t SpirvXfbBufferCount(const Blob& blob)
	{
		const void* pData = nullptr;
		size_t size = 0;
		GetVKBlobData(blob, &pData, &size);
		if (!pData || size < 20 || size % 4) return 0;

		const auto words = static_cast<const uint32_t*>(pData);
		const auto count = size / sizeof(uint32_t);
		if (words[0] != SpirvMagic) return 0;

		constexpr uint32_t OpExecutionMode = 16;
		constexpr uint32_t OpDecorate = 71;
		constexpr uint32_t OpMemberDecorate = 72;
		constexpr uint32_t ExecutionModeXfb = 29;
		constexpr uint32_t DecorationXfbBuffer = 35;

		auto hasXfb = false;
		auto buffers = 0u;
		for (auto i = 5u; i < count;)
		{
			const auto wordCount = words[i] >> 16;
			const auto opcode = words[i] & 0xffff;
			if (!wordCount || i + wordCount > count) break;

			if (opcode == OpExecutionMode && wordCount >= 3 && words[i + 2] == ExecutionModeXfb)
				hasXfb = true;
			else if (opcode == OpDecorate && wordCount >= 4 && words[i + 2] == DecorationXfbBuffer)
				buffers = (max)(buffers, words[i + 3] + 1);
			else if (opcode == OpMemberDecorate && wordCount >= 5 && words[i + 3] == DecorationXfbBuffer)
				buffers = (max)(buffers, words[i + 4] + 1);

			i += wordCount;
		}

		if (!hasXfb) return 0;

		return buffers ? buffers : 1u;
	}
}

void XUSG::InitGraphicsPresets(function<Graphics::Blend(uint8_t)>* pBlends,
	function<Graphics::Rasterizer()>* pRasterizers, function<Graphics::DepthStencil()>* pDepthStencils)
{
	pBlends[BlendPreset::DEFAULT_OPAQUE] = PipelineLib::DefaultOpaque;
	pBlends[BlendPreset::PREMULTIPLITED] = PipelineLib::Premultiplied;
	pBlends[BlendPreset::ADDTIVE] = PipelineLib::Additive;
	pBlends[BlendPreset::NON_PRE_MUL] = PipelineLib::NonPremultiplied;
	pBlends[BlendPreset::NON_PREMUL_RT0] = PipelineLib::NonPremultipliedRT0;
	pBlends[BlendPreset::ALPHA_TO_COVERAGE] = PipelineLib::AlphaToCoverage;
	pBlends[BlendPreset::ACCUMULATIVE] = PipelineLib::Accumulative;
	pBlends[BlendPreset::AUTO_NON_PREMUL] = PipelineLib::AutoNonPremultiplied;
	pBlends[BlendPreset::ZERO_ALPHA_PREMUL] = PipelineLib::ZeroAlphaNonPremultiplied;
	pBlends[BlendPreset::MULTIPLITED] = PipelineLib::Multiplied;
	pBlends[BlendPreset::WEIGHTED_PREMUL] = PipelineLib::WeightedPremul;
	pBlends[BlendPreset::WEIGHTED_PREMUL_PER_RT] = PipelineLib::WeightedPremulPerRT;
	pBlends[BlendPreset::WEIGHTED_PER_RT] = PipelineLib::WeightedPerRT;
	pBlends[BlendPreset::SELECT_MIN] = PipelineLib::SelectMin;
	pBlends[BlendPreset::SELECT_MAX] = PipelineLib::SelectMax;

	pRasterizers[RasterizerPreset::CULL_BACK] = PipelineLib::CullBack;
	pRasterizers[RasterizerPreset::CULL_NONE] = PipelineLib::CullNone;
	pRasterizers[RasterizerPreset::CULL_FRONT] = PipelineLib::CullFront;
	pRasterizers[RasterizerPreset::FILL_WIREFRAME] = PipelineLib::FillWireframe;
	pRasterizers[RasterizerPreset::CULL_BACK_CONSERVATIVE] = PipelineLib::CullBack_Conservative;
	pRasterizers[RasterizerPreset::CULL_NONE_CONSERVATIVE] = PipelineLib::CullNone_Conservative;
	pRasterizers[RasterizerPreset::CULL_FRONT_CONSERVATIVE] = PipelineLib::CullFront_Conservative;

	pDepthStencils[DepthStencilPreset::DEFAULT_LESS] = PipelineLib::DepthStencilDefault;
	pDepthStencils[DepthStencilPreset::DEPTH_STENCIL_NONE] = PipelineLib::DepthStencilNone;
	pDepthStencils[DepthStencilPreset::DEPTH_LESS_EQUAL] = PipelineLib::DepthLessEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_EQUAL] = PipelineLib::DepthEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_GREATER] = PipelineLib::DepthGreater;
	pDepthStencils[DepthStencilPreset::DEPTH_GREATER_EQUAL] = PipelineLib::DepthGreaterEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_NOT_EQUAL] = PipelineLib::DepthNotEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_ALWAYS] = PipelineLib::DepthAlways;
	pDepthStencils[DepthStencilPreset::DEPTH_READ_LESS] = PipelineLib::DepthReadLess;
	pDepthStencils[DepthStencilPreset::DEPTH_READ_LESS_EQUAL] = PipelineLib::DepthReadLessEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_READ_EQUAL] = PipelineLib::DepthReadEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_READ_GREATER] = PipelineLib::DepthReadGreater;
	pDepthStencils[DepthStencilPreset::DEPTH_READ_GREATER_EQUAL] = PipelineLib::DepthReadGreaterEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_READ_NOT_EQUAL] = PipelineLib::DepthReadNotEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_READ_NEVER] = PipelineLib::DepthReadNever;
	pDepthStencils[DepthStencilPreset::DEPTH_READ_ALWAYS] = PipelineLib::DepthReadAlways;
	pDepthStencils[DepthStencilPreset::DEPTH_LESS_STENCIL_ZERO] = PipelineLib::DepthLess_StencilZero;
	pDepthStencils[DepthStencilPreset::DEPTH_LESS_EQUAL_STENCIL_ZERO] = PipelineLib::DepthLessEqual_StencilZero;
	pDepthStencils[DepthStencilPreset::DEPTH_EQUAL_STENCIL_ZERO] = PipelineLib::DepthEqual_StencilZero;
	pDepthStencils[DepthStencilPreset::DEPTH_GREATER_STENCIL_ZERO] = PipelineLib::DepthGreater_StencilZero;
	pDepthStencils[DepthStencilPreset::DEPTH_GREATER_EQUAL_STENCIL_ZERO] = PipelineLib::DepthGreaterEqual_StencilZero;
	pDepthStencils[DepthStencilPreset::DEPTH_NOT_EQUAL_STENCIL_ZERO] = PipelineLib::DepthNotEqual_StencilZero;
	pDepthStencils[DepthStencilPreset::DEPTH_NEVER_STENCIL_ZERO] = PipelineLib::DepthNever_StencilZero;
	pDepthStencils[DepthStencilPreset::DEPTH_ALWAYS_STENCIL_ZERO] = PipelineLib::DepthAlways_StencilZero;
	pDepthStencils[DepthStencilPreset::DEPTH_LESS_STENCIL_REPLACE] = PipelineLib::DepthLess_StencilReplace;
	pDepthStencils[DepthStencilPreset::DEPTH_LESS_EQUAL_STENCIL_REPLACE] = PipelineLib::DepthLessEqual_StencilReplace;
	pDepthStencils[DepthStencilPreset::DEPTH_EQUAL_STENCIL_REPLACE] = PipelineLib::DepthEqual_StencilReplace;
	pDepthStencils[DepthStencilPreset::DEPTH_GREATER_STENCIL_REPLACE] = PipelineLib::DepthGreater_StencilReplace;
	pDepthStencils[DepthStencilPreset::DEPTH_GREATER_EQUAL_STENCIL_REPLACE] = PipelineLib::DepthGreaterEqual_StencilReplace;
	pDepthStencils[DepthStencilPreset::DEPTH_NOT_EQUAL_STENCIL_REPLACE] = PipelineLib::DepthNotEqual_StencilReplace;
	pDepthStencils[DepthStencilPreset::DEPTH_NEVER_STENCIL_REPLACE] = PipelineLib::DepthNever_StencilReplace;
	pDepthStencils[DepthStencilPreset::DEPTH_ALWAYS_STENCIL_REPLACE] = PipelineLib::DepthAlways_StencilReplace;
	pDepthStencils[DepthStencilPreset::DEPTH_LESS_STENCIL_EQUAL] = PipelineLib::DepthLess_StencilEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_LESS_EQUAL_STENCIL_EQUAL] = PipelineLib::DepthLessEqual_StencilEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_EQUAL_STENCIL_EQUAL] = PipelineLib::DepthEqual_StencilEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_GREATER_STENCIL_EQUAL] = PipelineLib::DepthGreater_StencilEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_GREATER_EQUAL_STENCIL_EQUAL] = PipelineLib::DepthGreaterEqual_StencilEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_NOT_EQUAL_STENCIL_EQUAL] = PipelineLib::DepthNotEqual_StencilEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_NEVER_STENCIL_EQUAL] = PipelineLib::DepthNever_StencilEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_ALWAYS_STENCIL_EQUAL] = PipelineLib::DepthAlways_StencilEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_LESS_STENCIL_NOT_EQUAL] = PipelineLib::DepthLess_StencilNotEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_LESS_EQUAL_STENCIL_NOT_EQUAL] = PipelineLib::DepthLessEqual_StencilNotEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_EQUAL_STENCIL_NOT_EQUAL] = PipelineLib::DepthEqual_StencilNotEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_GREATER_STENCIL_NOT_EQUAL] = PipelineLib::DepthGreater_StencilNotEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_GREATER_EQUAL_STENCIL_NOT_EQUAL] = PipelineLib::DepthGreaterEqual_StencilNotEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_NOT_EQUAL_STENCIL_NOT_EQUAL] = PipelineLib::DepthNotEqual_StencilNotEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_NEVER_STENCIL_NOT_EQUAL] = PipelineLib::DepthNever_StencilNotEqual;
	pDepthStencils[DepthStencilPreset::DEPTH_ALWAYS_STENCIL_NOT_EQUAL] = PipelineLib::DepthAlways_StencilNotEqual;
	pDepthStencils[DepthStencilPreset::STENCIL_FRONT_INC_BACK_DEC] = PipelineLib::StencilFrontIncrBackDecr;
	pDepthStencils[DepthStencilPreset::STENCIL_FRONT_DEC_BACK_INC] = PipelineLib::StencilFrontDecrBackIncr;
	pDepthStencils[DepthStencilPreset::STENCIL_FRONT_PASS_ZERO] = PipelineLib::StencilPassZero;
	pDepthStencils[DepthStencilPreset::STENCIL_FRONT_FAIL_ZERO] = PipelineLib::StencilFailZero;
	pDepthStencils[DepthStencilPreset::STENCIL_FRONT_PASS_INVERT] = PipelineLib::StencilPassInvert;
	pDepthStencils[DepthStencilPreset::STENCIL_FRONT_FAIL_INVERT] = PipelineLib::StencilFailInvert;
}

Pipeline_VK* XUSG::CreateVKGraphicsPipeline(const Device_VK* pDevice, const GraphicsPipelineCreateVK& desc)
{
	assert(pDevice);
	const auto device = pDevice->GetDevice();
	const auto layoutObject = desc.pLayout;

	VkPipelineShaderStageCreateInfo stages[Shader::NUM_STAGE] = {};
	VkShaderModule modules[Shader::NUM_STAGE] = {};
	string entries[Shader::NUM_STAGE];
	auto stageCount = 0u;
	auto hasTessellation = false;
	for (auto i = 0u; i < desc.ShaderCount; ++i)
	{
		const auto stage = static_cast<Shader::Stage>(i);
		if (stage == Shader::CS || !desc.pShaders || !desc.pShaders[i]) continue;

		modules[stageCount] = CreateShaderModule(device, desc.pShaders[i]);
		if (desc.pEntryPoints && desc.pEntryPoints[i]) entries[stageCount] = desc.pEntryPoints[i];
		else entries[stageCount] = "main";
		stages[stageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[stageCount].pNext = layoutObject && layoutObject->StageMappingInfos[stage].mappingCount ?
			&layoutObject->StageMappingInfos[stage] : nullptr;
		stages[stageCount].stage = GetVKShaderStage(stage);
		stages[stageCount].module = modules[stageCount];
		stages[stageCount].pName = entries[stageCount].c_str();
		if (stage == Shader::HS || stage == Shader::DS) hasTessellation = true;
		++stageCount;
	}

	// Input layout
	vector<VkVertexInputBindingDescription> bindings;
	vector<VkVertexInputAttributeDescription> attributes;
	vector<VkVertexInputBindingDivisorDescription> divisors;
	vector<uint32_t> attributeSizes;
	if (desc.pInputLayout)
	{
		auto offset = 0u;
		for (auto i = 0u; i < desc.pInputLayout->size(); ++i)
		{
			const auto& element = (*desc.pInputLayout)[i];
			const auto binding = element.InputSlot;
			auto found = false;
			for (const auto& existing : bindings)
				if (existing.binding == binding) found = true;
			if (!found)
			{
				VkVertexInputBindingDescription input = {};
				input.binding = binding;
				input.inputRate = element.InputSlotClass == InputClassification::PER_INSTANCE_DATA ?
					VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
				bindings.push_back(input);
			}
			if (element.InputSlotClass == InputClassification::PER_INSTANCE_DATA && element.InstanceDataStepRate != 1)
			{
				assert(element.InstanceDataStepRate);
				auto foundDivisor = false;
				for (auto& divisor : divisors)
				{
					if (divisor.binding != binding) continue;
					assert(divisor.divisor == element.InstanceDataStepRate);
					foundDivisor = true;
				}
				if (!foundDivisor)
				{
					VkVertexInputBindingDivisorDescription divisor = {};
					divisor.binding = binding;
					divisor.divisor = element.InstanceDataStepRate;
					divisors.push_back(divisor);
				}
			}
			const auto sizeInBytes = (max)(GetVKFormatBits(element.Format), 8u) / 8;
			VkVertexInputAttributeDescription attribute = {};
			attribute.location = i;
			attribute.binding = binding;
			attribute.format = GetVKFormat(element.Format);
			attribute.offset = element.AlignedByteOffset == UINT32_MAX ? offset : element.AlignedByteOffset;
			attributes.push_back(attribute);
			attributeSizes.push_back(sizeInBytes);
			offset = attribute.offset + sizeInBytes;
		}
		for (auto& binding : bindings)
		{
			auto stride = 0u;
			for (auto i = 0u; i < attributes.size(); ++i)
				if (attributes[i].binding == binding.binding)
					stride = (max)(stride, attributes[i].offset + attributeSizes[i]);
			binding.stride = stride ? stride : 4;
		}
	}

	VkPipelineVertexInputStateCreateInfo vertex = {};
	vertex.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
	vertex.pVertexBindingDescriptions = bindings.empty() ? nullptr : bindings.data();
	vertex.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
	vertex.pVertexAttributeDescriptions = attributes.empty() ? nullptr : attributes.data();
	VkPipelineVertexInputDivisorStateCreateInfo divisorState = {};
	divisorState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO;
	divisorState.vertexBindingDivisorCount = static_cast<uint32_t>(divisors.size());
	divisorState.pVertexBindingDivisors = divisors.empty() ? nullptr : divisors.data();
	if (!divisors.empty())
	{
		const auto vertexAttributeDivisor = pDevice->HasVertexAttributeDivisor();
		assert(vertexAttributeDivisor);
		if (vertexAttributeDivisor) vertex.pNext = &divisorState;
	}

	VkPipelineInputAssemblyStateCreateInfo assembly = {};
	assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assembly.topology = GetVKPrimitiveTopology(desc.Topology);

	auto patchControlPoints = 0u;
	if (hasTessellation && desc.pShaders && desc.pShaders[Shader::HS])
	{
		const auto entry = desc.pEntryPoints && desc.pEntryPoints[Shader::HS] ? desc.pEntryPoints[Shader::HS] : "main";
		patchControlPoints = SpirvInputControlPointCount(desc.pShaders[Shader::HS], entry);
	}
	assert(!hasTessellation || patchControlPoints);
	VkPipelineTessellationStateCreateInfo tessellation = {};
	tessellation.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessellation.patchControlPoints = patchControlPoints ? patchControlPoints : 1;

	VkPipelineViewportStateCreateInfo viewport = {};
	viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport.viewportCount = 1;
	viewport.scissorCount = 1;

	// Blend state
	const auto blendDesc = desc.pBlend ? *desc.pBlend : Graphics::PipelineLib::DefaultOpaque(desc.NumRenderTargets);
	VkPipelineColorBlendAttachmentState attachments[8] = {};
	for (auto i = 0u; i < 8u; ++i)
	{
		const auto& src = blendDesc.RenderTargets[blendDesc.IndependentBlendEnable ? i : 0];
		attachments[i].blendEnable = src.BlendEnable;
		attachments[i].srcColorBlendFactor = GetVKBlendFactor(src.SrcBlend);
		attachments[i].dstColorBlendFactor = GetVKBlendFactor(src.DestBlend);
		attachments[i].colorBlendOp = GetVKBlendOp(src.BlendOp);
		attachments[i].srcAlphaBlendFactor = GetVKBlendFactor(src.SrcBlendAlpha);
		attachments[i].dstAlphaBlendFactor = GetVKBlendFactor(src.DestBlendAlpha);
		attachments[i].alphaBlendOp = GetVKBlendOp(src.BlendOpAlpha);
		attachments[i].colorWriteMask = static_cast<VkColorComponentFlags>(src.WriteMask);
	}
	VkPipelineColorBlendStateCreateInfo blend = {};
	blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.logicOpEnable = blendDesc.RenderTargets[0].LogicOpEnable;
	blend.logicOp = GetVKLogicOp(blendDesc.RenderTargets[0].LogicOp);
	blend.attachmentCount = desc.NumRenderTargets;
	blend.pAttachments = attachments;

	// Rasterizer state
	const auto rasterizer = desc.pRasterizer ? *desc.pRasterizer : Graphics::PipelineLib::CullBack();
	VkPipelineRasterizationStateCreateInfo raster = {};
	raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster.depthClampEnable = !rasterizer.DepthClipEnable;
	raster.polygonMode = GetVKPolygonMode(rasterizer.Fill);
	raster.cullMode = GetVKCullMode(rasterizer.Cull);
	raster.frontFace = rasterizer.FrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
	raster.depthBiasEnable = rasterizer.DepthBias != 0.0f || rasterizer.SlopeScaledDepthBias != 0.0f;
	raster.depthBiasConstantFactor = rasterizer.DepthBias;
	raster.depthBiasClamp = rasterizer.DepthBiasClamp;
	raster.depthBiasSlopeFactor = rasterizer.SlopeScaledDepthBias;
	raster.lineWidth = 1.0f;
	VkPipelineRasterizationConservativeStateCreateInfoEXT conservative = {};
	conservative.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
	conservative.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
	if (rasterizer.ConservativeRaster)
	{
		const auto conservativeRasterization = pDevice->HasConservativeRasterization();
		assert(conservativeRasterization);
		if (conservativeRasterization) raster.pNext = &conservative;
	}

	const auto sampleCount = desc.SampleCount ? desc.SampleCount : 1u;
	VkSampleLocationEXT standardLocations[16];
	for (auto i = 0u; i < sampleCount && i < 16u; ++i)
	{
		standardLocations[i].x = 0.5f;
		standardLocations[i].y = 0.5f;
	}
	VkPipelineSampleLocationsStateCreateInfoEXT sampleLocations = {};
	sampleLocations.sType = VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT;
	sampleLocations.sampleLocationsEnable = VK_FALSE;
	sampleLocations.sampleLocationsInfo.sType = VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT;
	sampleLocations.sampleLocationsInfo.sampleLocationsPerPixel = static_cast<VkSampleCountFlagBits>(sampleCount);
	sampleLocations.sampleLocationsInfo.sampleLocationGridSize = { 1, 1 };
	sampleLocations.sampleLocationsInfo.sampleLocationsCount = sampleCount;
	sampleLocations.sampleLocationsInfo.pSampleLocations = standardLocations;

	VkPipelineMultisampleStateCreateInfo multisample = {};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	const auto hasSampleLocations = pDevice->HasSampleLocations();
	multisample.pNext = hasSampleLocations ? &sampleLocations : nullptr;
	multisample.rasterizationSamples = static_cast<VkSampleCountFlagBits>(sampleCount);
	multisample.alphaToCoverageEnable = blendDesc.AlphaToCoverageEnable;

	// Depth-stencil state
	const auto depthStencilDesc = desc.pDepthStencil ? *desc.pDepthStencil : Graphics::PipelineLib::DepthStencilNone();
	VkPipelineDepthStencilStateCreateInfo depth = {};
	depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth.depthTestEnable = depthStencilDesc.DepthEnable;
	depth.depthWriteEnable = depthStencilDesc.DepthWriteMask;
	depth.depthCompareOp = GetVKCompareOp(depthStencilDesc.Comparison);
	depth.depthBoundsTestEnable = depthStencilDesc.DepthBoundsTestEnable;
	depth.stencilTestEnable = depthStencilDesc.StencilEnable;
	depth.front.failOp = GetVKStencilOp(depthStencilDesc.FrontFace.StencilFailOp);
	depth.front.passOp = GetVKStencilOp(depthStencilDesc.FrontFace.StencilPassOp);
	depth.front.depthFailOp = GetVKStencilOp(depthStencilDesc.FrontFace.StencilDepthFailOp);
	depth.front.compareOp = GetVKCompareOp(depthStencilDesc.FrontFace.StencilFunc);
	depth.front.compareMask = depthStencilDesc.FrontFace.StencilReadMask;
	depth.front.writeMask = depthStencilDesc.FrontFace.StencilWriteMask;
	depth.back.failOp = GetVKStencilOp(depthStencilDesc.BackFace.StencilFailOp);
	depth.back.passOp = GetVKStencilOp(depthStencilDesc.BackFace.StencilPassOp);
	depth.back.depthFailOp = GetVKStencilOp(depthStencilDesc.BackFace.StencilDepthFailOp);
	depth.back.compareOp = GetVKCompareOp(depthStencilDesc.BackFace.StencilFunc);
	depth.back.compareMask = depthStencilDesc.BackFace.StencilReadMask;
	depth.back.writeMask = depthStencilDesc.BackFace.StencilWriteMask;

	VkDynamicState dynamics[8] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_BLEND_CONSTANTS,
		VK_DYNAMIC_STATE_STENCIL_REFERENCE,
		VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY
	};
	auto dynamicCount = 5u;
	if (pDevice->GetBindVertexBuffers2()) dynamics[dynamicCount++] = VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE;
	if (hasSampleLocations)
	{
		dynamics[dynamicCount++] = VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT;
		dynamics[dynamicCount++] = VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_ENABLE_EXT;
	}
	VkPipelineDynamicStateCreateInfo dynamic = {};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic.dynamicStateCount = dynamicCount;
	dynamic.pDynamicStates = dynamics;

	VkFormat colorFormats[8] = {};
	for (auto i = 0u; i < desc.NumRenderTargets; ++i) colorFormats[i] = GetVKFormat(desc.RTVFormats[i]);
	VkPipelineRenderingCreateInfo rendering = {};
	rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	rendering.colorAttachmentCount = desc.NumRenderTargets;
	rendering.pColorAttachmentFormats = desc.NumRenderTargets ? colorFormats : nullptr;
	rendering.depthAttachmentFormat = GetVKFormat(pDevice, desc.DSVFormat);
	rendering.stencilAttachmentFormat = IsDepthStencilFormat(desc.DSVFormat) &&
		desc.DSVFormat != Format::D32_FLOAT && desc.DSVFormat != Format::D16_UNORM ?
		rendering.depthAttachmentFormat : VK_FORMAT_UNDEFINED;

	VkPipelineCreateFlags2CreateInfo flags = {};
	flags.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO;
	flags.pNext = &rendering;
	flags.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT;

	VkGraphicsPipelineCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.pNext = &flags;
	info.stageCount = stageCount;
	info.pStages = stages;
	info.pVertexInputState = &vertex;
	info.pInputAssemblyState = &assembly;
	info.pTessellationState = hasTessellation ? &tessellation : nullptr;
	info.pViewportState = &viewport;
	info.pRasterizationState = &raster;
	info.pMultisampleState = &multisample;
	info.pDepthStencilState = &depth;
	info.pColorBlendState = &blend;
	info.pDynamicState = &dynamic;
	info.layout = VK_NULL_HANDLE;

	VkPipeline handle = VK_NULL_HANDLE;
	const auto result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &handle);
	for (auto i = 0u; i < stageCount; ++i)
		if (modules[i] != VK_NULL_HANDLE) vkDestroyShaderModule(device, modules[i], nullptr);
	if (result < 0)
	{
		cerr << "VkResult " << result << endl;

		return nullptr;
	}

	auto xfbBuffers = 0u;
	if (desc.pShaders)
	{
		const Shader::Stage lastStage[] = { Shader::MS, Shader::GS, Shader::DS, Shader::VS };
		for (const auto stage : lastStage)
		{
			if (static_cast<uint32_t>(stage) >= desc.ShaderCount || !desc.pShaders[stage]) continue;
			xfbBuffers = SpirvXfbBufferCount(desc.pShaders[stage]);
			break;
		}
	}
	if (xfbBuffers > 4) xfbBuffers = 4;

	auto pipeline = new Pipeline_VK();
	pipeline->Handle = handle;
	pipeline->BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	pipeline->Refs = 1;
	pipeline->XfbBuffers = xfbBuffers;
	pDevice->SetDebugName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)handle, desc.Name);

	return pipeline;
}

Pipeline_VK* XUSG::CreateVKComputePipeline(const Device_VK* pDevice, const PipelineLayoutObject* pLayout,
	const Blob& shader, const char* entryPoint, const wchar_t* name)
{
	assert(pDevice);
	const auto device = pDevice->GetDevice();
	const auto module = CreateShaderModule(device, shader);
	const string entry = entryPoint ? entryPoint : "main";

	VkPipelineShaderStageCreateInfo stage = {};
	stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage.pNext = pLayout && pLayout->StageMappingInfos[Shader::CS].mappingCount ?
		&pLayout->StageMappingInfos[Shader::CS] : nullptr;
	stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage.module = module;
	stage.pName = entry.c_str();

	VkPipelineCreateFlags2CreateInfo flags = {};
	flags.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO;
	flags.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT;

	VkComputePipelineCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.pNext = &flags;
	info.stage = stage;
	info.layout = VK_NULL_HANDLE;

	VkPipeline handle = VK_NULL_HANDLE;
	const auto result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &handle);
	if (module != VK_NULL_HANDLE) vkDestroyShaderModule(device, module, nullptr);
	if (result < 0)
	{
		cerr << "VkResult " << result << endl;

		return nullptr;
	}

	auto pipeline = new Pipeline_VK();
	pipeline->Handle = handle;
	pipeline->BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	pipeline->Refs = 1;
	pDevice->SetDebugName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)handle, name);

	return pipeline;
}

Pipeline PipelineLib_VK::createPipeline(const string& key, const wchar_t* name)
{
	const auto pDesc = reinterpret_cast<const PipelineDesc*>(key.data());
	GraphicsPipelineCreateVK desc = {};
	desc.pLayout = static_cast<const PipelineLayoutObject*>(pDesc->Layout);
	desc.pShaders = pDesc->Shaders;
	desc.ShaderCount = Shader::Stage::NUM_GRAPHICS;
	desc.pBlend = pDesc->pBlend;
	desc.pRasterizer = pDesc->pRasterizer;
	desc.pDepthStencil = pDesc->pDepthStencil;
	desc.pInputLayout = pDesc->pInputLayout;
	desc.Topology = pDesc->PrimTopologyType;
	desc.NumRenderTargets = pDesc->NumRenderTargets;
	desc.RTVFormats = pDesc->RTVFormats;
	desc.DSVFormat = pDesc->DSVFormat;
	desc.SampleCount = pDesc->SampleCount;
	desc.Name = name;
	auto pipeline = CreateVKGraphicsPipeline(m_pDevice, desc);
	if (!pipeline) return nullptr;

	const auto existing = m_pipelines.find(key);
	if (existing != m_pipelines.end()) ReleaseVKPipeline(m_pDevice->GetDevice(), existing->second);
	m_pipelines[key] = pipeline;

	return pipeline;
}

Pipeline PipelineLib_VK::getPipeline(const string& key, const wchar_t* name)
{
	const auto pipeline = m_pipelines.find(key);
	if (pipeline == m_pipelines.cend()) return createPipeline(key, name);

	return pipeline->second;
}
