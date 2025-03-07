//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef HLSL_VERSION

#pragma once

#endif

//--------------------------------------------------------------------------------------
// Shader-shared constants
//--------------------------------------------------------------------------------------

#ifndef _XUSG_SHARED_CONST_H_
#define _XUSG_SHARED_CONST_H_

#ifndef XUSG_FRAME_COUNT
#define XUSG_FRAME_COUNT	3
#endif

#ifndef XUSG_TEMPORAL_AA
#define XUSG_TEMPORAL_AA	1
#endif

#ifndef XUSG_TEMPORAL
#define XUSG_TEMPORAL		XUSG_TEMPORAL_AA
#endif

#ifndef XUSG_NUM_CASCADE
#define XUSG_NUM_CASCADE	3
#endif

#define XUSG_PIDIV4			0.785398163f

static const float	XUSG_FOVAngleY = XUSG_PIDIV4;
static const float	XUSG_zNear = 1.0f;
static const float	XUSG_zFar = 1000.0f;

#endif

#if !(defined(HLSL_VERSION) || defined(__HLSL_VERSION))

#include "Core/XUSG.h"

namespace XUSG
{
	//--------------------------------------------------------------------------------------
	// DDS loader
	//--------------------------------------------------------------------------------------
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

	//--------------------------------------------------------------------------------------
	// SDKMesh class. This class reads the sdkmesh file format
	//--------------------------------------------------------------------------------------
	enum SubsetFlags : uint8_t
	{
		SUBSET_OPAQUE = 0x1,
		SUBSET_ALPHA = 0x2,
		SUBSET_ALPHA_TEST = 0x4,
		SUBSET_FULL = SUBSET_OPAQUE | SUBSET_ALPHA | SUBSET_ALPHA_TEST,
		SUBSET_HAS_ALPHA = SUBSET_ALPHA | SUBSET_ALPHA_TEST,

		NUM_SUBSET_TYPE = 2
	};

	XUSG_DEF_ENUM_FLAG_OPERATORS(SubsetFlags);

	struct TextureRecord
	{
		Texture::sptr Texture;
		uint8_t AlphaMode;
	};
	using TextureLib = std::shared_ptr<std::map<std::string, TextureRecord>>;

	class XUSG_INTERFACE SDKMesh
	{
	public:
		static const uint32_t MAX_VERTEX_STREAMS	= 16;
		static const uint32_t MAX_FRAME_NAME		= 100;
		static const uint32_t MAX_MESH_NAME			= 100;
		static const uint32_t MAX_SUBSET_NAME		= 100;
		static const uint32_t MAX_MATERIAL_NAME		= 100;
		static const uint32_t MAX_TEXTURE_NAME		= MAX_PATH;
		static const uint32_t MAX_MATERIAL_PATH		= MAX_PATH;

		enum IndexType
		{
			IT_16BIT = 0,
			IT_32BIT
		};

		enum PrimitiveType
		{
			PT_TRIANGLE_LIST = 0,
			PT_TRIANGLE_STRIP,
			PT_LINE_LIST,
			PT_LINE_STRIP,
			PT_POINT_LIST,
			PT_TRIANGLE_LIST_ADJ,
			PT_TRIANGLE_STRIP_ADJ,
			PT_LINE_LIST_ADJ,
			PT_LINE_STRIP_ADJ,
			PT_QUAD_PATCH_LIST,
			PT_TRIANGLE_PATCH_LIST
		};

#pragma pack(push, 8)
		struct Data
		{
			char Name[MAX_MESH_NAME];
			uint8_t NumVertexBuffers;
			uint32_t VertexBuffers[MAX_VERTEX_STREAMS];
			uint32_t IndexBuffer;
			uint32_t NumSubsets;
			uint32_t NumFrameInfluences; // aka bones

			DirectX::XMFLOAT3 BoundingBoxCenter;
			DirectX::XMFLOAT3 BoundingBoxExtents;

			union
			{
				uint64_t SubsetOffset;			// Offset to list of subsets (This also forces the union to 64bits)
				uint32_t* pSubsets;				// Pointer to list of subsets
			};
			union
			{
				uint64_t FrameInfluenceOffset;	// Offset to list of frame influences (This also forces the union to 64bits)
				uint32_t* pFrameInfluences;		// Pointer to list of frame influences
			};
		};

		struct Subset
		{
			char Name[MAX_SUBSET_NAME];
			uint32_t MaterialID;
			uint32_t PrimitiveType;
			uint64_t IndexStart;
			uint64_t IndexCount;
			uint64_t VertexStart;
			uint64_t VertexCount;
		};

		struct Frame
		{
			char Name[MAX_FRAME_NAME];
			uint32_t Mesh;
			uint32_t ParentFrame;
			uint32_t ChildFrame;
			uint32_t SiblingFrame;
			DirectX::XMFLOAT4X4 Matrix;
			uint32_t AnimationDataIndex;		// Used to index which set of keyframes transforms this frame
		};

		struct Material
		{
			char Name[MAX_MATERIAL_NAME];

			// Use MaterialInstancePath
			char MaterialInstancePath[MAX_MATERIAL_PATH];

			// Or fall back to d3d8-type materials
			char AlbedoTexture[MAX_TEXTURE_NAME];
			char NormalTexture[MAX_TEXTURE_NAME];
			char SpecularTexture[MAX_TEXTURE_NAME];

			DirectX::XMFLOAT4 Albedo;
			DirectX::XMFLOAT4 Ambient;
			DirectX::XMFLOAT4 Specular;
			DirectX::XMFLOAT4 Emissive;
			float Power;

			union
			{
				uint64_t Albedo64;			//Force the union to 64bits
				ShaderResource* pAlbedo;
			};
			union
			{
				uint64_t Normal64;			//Force the union to 64bits
				ShaderResource* pNormal;
			};
			union
			{
				uint64_t Specular64;		//Force the union to 64bits
				ShaderResource* pSpecular;
			};
			uint64_t AlphaModeAlbedo;		// Force the union to 64bits
			uint64_t AlphaModeNormal;		// Force the union to 64bits
			uint64_t AlphaModeSpecular;		// Force the union to 64bits
		};

		struct AnimationFileHeader
		{
			uint32_t	Version;
			uint8_t		IsBigEndian;
			uint32_t	FrameTransformType;
			uint32_t	NumFrames;
			uint32_t	NumAnimationKeys;
			uint32_t	AnimationFPS;
			uint64_t	AnimationDataSize;
			uint64_t	AnimationDataOffset;
		};

		struct AnimationData
		{
			DirectX::XMFLOAT3 Translation;
			DirectX::XMFLOAT4 Orientation;
			DirectX::XMFLOAT3 Scaling;
		};

		struct AnimationFrameData
		{
			char FrameName[MAX_FRAME_NAME];
			union
			{
				uint64_t DataOffset;
				AnimationData* pAnimationData;
			};
		};
#pragma pack(pop)

		virtual ~SDKMesh() {};

		virtual bool Create(const Device* pDevice, const wchar_t* fileName,
			const TextureLib& textureLib, bool isStaticMesh = false) = 0;
		virtual bool Create(const Device* pDevice, uint8_t* pData, const TextureLib& textureLib,
			size_t dataBytes, bool isStaticMesh = false, bool copyStatic = false) = 0;
		virtual bool LoadAnimation(const wchar_t* fileName) = 0;
		virtual void Destroy() = 0;

		//Frame manipulation
		virtual void TransformBindPose(DirectX::CXMMATRIX world) = 0;
		virtual void TransformMesh(DirectX::CXMMATRIX world, double time) = 0;

		// Helpers (Graphics API specific)
		static PrimitiveTopology GetPrimitiveType(PrimitiveType primType);
		virtual Format GetIBFormat(uint32_t mesh) const = 0;

		virtual IndexType GetIndexType(uint32_t mesh) const = 0;

		virtual const Descriptor&		GetVertexBufferSRV(uint32_t mesh, uint32_t i) const = 0;
		virtual const VertexBufferView&	GetVertexBufferView(uint32_t mesh, uint32_t i) const = 0;
		virtual const IndexBufferView&	GetIndexBufferView(uint32_t mesh) const = 0;
		virtual const IndexBufferView&	GetAdjIndexBufferView(uint32_t mesh) const = 0;

		virtual const Descriptor&		GetVertexBufferSRVAt(uint32_t vb) const = 0;
		virtual const VertexBufferView&	GetVertexBufferViewAt(uint32_t vb) const = 0;
		virtual const IndexBufferView&	GetIndexBufferViewAt(uint32_t ib) const = 0;

		// Helpers (general)
		virtual const char*			GetMeshPathA() const = 0;
		virtual const wchar_t*		GetMeshPathW() const = 0;
		virtual uint32_t			GetNumMeshes() const = 0;
		virtual uint32_t			GetNumMaterials() const = 0;
		virtual uint32_t			GetNumVertexBuffers() const = 0;
		virtual uint32_t			GetNumIndexBuffers() const = 0;

		virtual uint8_t*			GetRawVerticesAt(uint32_t vb) const = 0;
		virtual uint8_t*			GetRawIndicesAt(uint32_t ib) const = 0;

		virtual Material*			GetMaterial(uint32_t material) const = 0;
		virtual Data*				GetMesh(uint32_t mesh) const = 0;
		virtual uint32_t			GetNumSubsets(uint32_t mesh) const = 0;
		virtual uint32_t			GetNumSubsets(uint32_t mesh, SubsetFlags materialType) const = 0;
		virtual Subset*				GetSubset(uint32_t mesh, uint32_t subset) const = 0;
		virtual Subset*				GetSubset(uint32_t mesh, uint32_t subset, SubsetFlags materialType) const = 0;
		virtual uint32_t			GetVertexStride(uint32_t mesh, uint32_t i) const = 0;
		virtual uint32_t			GetNumFrames() const = 0;
		virtual Frame*				GetFrame(uint32_t frame) const = 0;
		virtual Frame*				FindFrame(const char* name) const = 0;
		virtual uint32_t			FindFrameIndex(const char* name) const = 0;
		virtual uint64_t			GetNumVertices(uint32_t mesh, uint32_t i) const = 0;
		virtual uint64_t			GetNumIndices(uint32_t mesh) const = 0;
		virtual DirectX::XMVECTOR	GetMeshBBoxCenter(uint32_t mesh) const = 0;
		virtual DirectX::XMVECTOR	GetMeshBBoxExtents(uint32_t mesh) const = 0;
		virtual uint32_t			GetOutstandingResources() const = 0;
		virtual uint32_t			GetOutstandingBufferResources() const = 0;
		virtual bool				CheckLoadDone() = 0;
		virtual bool				IsLoaded() const = 0;
		virtual bool				IsLoading() const = 0;
		virtual void				SetLoading(bool loading) = 0;
		virtual bool				HadLoadingError() const = 0;

		// Animation
		virtual uint32_t			GetNumInfluences(uint32_t mesh) const = 0;
		virtual DirectX::XMMATRIX	GetMeshInfluenceMatrix(uint32_t mesh, uint32_t influence) const = 0;
		virtual uint32_t			GetAnimationKeyFromTime(double time) const = 0;
		virtual DirectX::XMMATRIX	GetWorldMatrix(uint32_t frameIndex) const = 0;
		virtual DirectX::XMMATRIX	GetInfluenceMatrix(uint32_t frameIndex) const = 0;
		virtual DirectX::XMMATRIX	GetBindMatrix(uint32_t frameIndex) const = 0;
		virtual bool				GetAnimationProperties(uint32_t* pNumKeys, float* pFrameTime) const = 0;

		using uptr = std::unique_ptr<SDKMesh>;
		using sptr = std::shared_ptr<SDKMesh>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Model base
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE Model
	{
	public:
		enum PipelineLayoutIndex : uint8_t
		{
			BASE_PASS,
			DEPTH_PASS,
			DEPTH_ALPHA_PASS,

			NUM_PIPELINE_LAYOUT
		};

		enum PipelineIndex : uint8_t
		{
			OPAQUE_FRONT,
			OPAQUE_TWO_SIDED,
			ALPHA_TWO_SIDED,
			ALPHA_TEST_TWO_SIDED,
			DEPTH_FRONT,
			DEPTH_TWO_SIDED,
			DEPTH_ALPHA_TWO_SIDED,
			SHADOW_FRONT,
			SHADOW_TWO_SIDED,
			SHADOW_ALPHA_TWO_SIDED,

			NUM_PIPELINE
		};

		enum DescriptorTableSlot : uint8_t
		{
			MATRICES,
			PER_FRAME,
#if XUSG_TEMPORAL_AA
			TEMPORAL_BIAS,
#endif
			VARIABLE_SLOT
		};

		enum DescriptorTableSlotOffset : uint8_t
		{
			MATERIAL_OFFSET,
			ALPHA_REF_OFFSET,
			IMMUTABLE_OFFSET = ALPHA_REF_OFFSET
		};

		enum CBVTableIndex : uint8_t
		{
			CBV_MATRICES,
#if XUSG_TEMPORAL_AA
			CBV_LOCAL_TEMPORAL_BIAS,
#endif

			NUM_CBV_TABLE
		};

		//Model(const wchar_t* name);
		virtual ~Model() {};

		virtual bool Init(const Device* pDevice, const InputLayout* pInputLayout,
			const SDKMesh::sptr& mesh, const ShaderLib::sptr& shaderLib,
			const Graphics::PipelineLib::sptr& pipelineLib,
			const PipelineLayoutLib::sptr& pipelineLayoutLib,
			const DescriptorTableLib::sptr& descriptorTableLib,
			bool twoSidedAll) = 0;
		virtual bool CreateDescriptorTables() = 0;

		virtual void Update(uint8_t frameIndex) = 0;
		virtual void SetMatrices(DirectX::CXMMATRIX world, bool isTemporal = true) = 0;
#if XUSG_TEMPORAL_AA
		virtual void SetTemporalBias(const DirectX::XMFLOAT2& temporalBias) = 0;
#endif
		virtual void SetPipelineLayout(const CommandList* pCommandList, PipelineLayoutIndex layout) = 0;
		virtual void SetPipeline(const CommandList* pCommandList, PipelineIndex pipeline) = 0;
		virtual void SetPipeline(const CommandList* pCommandList, SubsetFlags subsetFlag, PipelineLayoutIndex layout) = 0;
		virtual void Render(const CommandList* pCommandList, SubsetFlags subsetFlags, uint8_t matrixTableIndex,
			PipelineLayoutIndex layout = NUM_PIPELINE_LAYOUT, const DescriptorTable* pCbvPerFrameTable = nullptr,
			uint32_t numInstances = 1) = 0;

		virtual bool IsTwoSidedAll() const = 0;

		Model* AsModel();

		static const InputLayout* CreateInputLayout(Graphics::PipelineLib* pPipelineLib);
		static std::shared_ptr<SDKMesh> LoadSDKMesh(const Device* pDevice, const std::wstring& meshFileName,
			const TextureLib& textureLib, bool isStaticMesh, API api);

		static constexpr uint8_t GetFrameCount() { return FrameCount; }

		using uptr = std::unique_ptr<Model>;
		using sptr = std::shared_ptr<Model>;

		static uptr MakeUnique(const wchar_t* name, API api);
		static sptr MakeShared(const wchar_t* name, API api);

	protected:
		static const uint8_t FrameCount = XUSG_FRAME_COUNT;
	};

	//--------------------------------------------------------------------------------------
	// Character model
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE Character :
		public virtual Model
	{
	public:
		enum DescriptorTableSlot : uint8_t
		{
			MATERIAL = VARIABLE_SLOT,
			ALPHA_REF,
			SHADOW_MAP,
			IMMUTABLE = ALPHA_REF,
#if XUSG_TEMPORAL
			HISTORY = ALPHA_REF
#endif
		};

		struct MeshLink
		{
			std::wstring	MeshName;
			std::string		BoneName;
			uint32_t		BoneIndex;
		};

		//Character(const wchar_t* name = nullptr, API api = API::DIRECTX_12);
		virtual ~Character() {};

		virtual bool Init(const Device* pDevice, const InputLayout* pInputLayout,
			const SDKMesh::sptr& mesh, const ShaderLib::sptr& shaderLib,
			const Graphics::PipelineLib::sptr& graphicsPipelineLib,
			const Compute::PipelineLib::sptr& computePipelineLib,
			const PipelineLayoutLib::sptr& pipelineLayoutLib,
			const DescriptorTableLib::sptr& descriptorTableLib,
			const std::shared_ptr<std::vector<SDKMesh>>& linkedMeshes = nullptr,
			const std::shared_ptr<std::vector<MeshLink>>& meshLinks = nullptr,
			const Format* rtvFormats = nullptr, uint32_t numRTVs = 0,
			Format dsvFormat = Format::UNKNOWN, Format shadowFormat = Format::UNKNOWN,
			bool twoSidedAll = false, bool useZEqual = true) = 0;
		virtual bool CreateDescriptorTables() = 0;

		virtual void InitPosition(const DirectX::XMFLOAT4& posRot) = 0;
		virtual void Update(uint8_t frameIndex, double time) = 0;
		virtual void Update(uint8_t frameIndex, double time, DirectX::FXMMATRIX* pWorld, bool isTemporal = true) = 0;
		virtual void SetMatrices(DirectX::FXMMATRIX* pWorld = nullptr, bool isTemporal = true) = 0;
		virtual void SetSkinningPipeline(const CommandList* pCommandList) = 0;
		virtual void Skinning(CommandList* pCommandList, uint32_t& numBarriers,
			ResourceBarrier* pBarriers, bool reset = false) = 0;
		virtual void RenderTransformed(const CommandList* pCommandList, PipelineLayoutIndex layout,
			SubsetFlags subsetMask = SUBSET_FULL, const DescriptorTable* pCbvPerFrameTable = nullptr,
			uint32_t numInstances = 1) = 0;

		virtual const DirectX::XMFLOAT4& GetPosition() const = 0;
		virtual DirectX::FXMMATRIX GetWorldMatrix() const = 0;

		static SDKMesh::sptr LoadSDKMesh(const Device* pDevice, const std::wstring& meshFileName,
			const std::wstring& animFileName, const TextureLib& textureLib,
			const std::shared_ptr<std::vector<MeshLink>>& meshLinks = nullptr,
			std::vector<SDKMesh::sptr>* pLinkedMeshes = nullptr, API api = API::DIRECTX_12);

		using uptr = std::unique_ptr<Character>;
		using sptr = std::shared_ptr<Character>;

		static uptr MakeUnique(const wchar_t* name = nullptr, API api = API::DIRECTX_12);
		static sptr MakeShared(const wchar_t* name = nullptr, API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Static model
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE StaticModel :
		public virtual Model
	{
	public:
		enum DescriptorTableSlot : uint8_t
		{
			PER_OBJECT = VARIABLE_SLOT,
			MATERIAL,
			ALPHA_REF,
			SHADOW_MAP,
			IMMUATABLE = ALPHA_REF
		};

		//StaticModel(const wchar_t* name = nullptr);
		virtual ~StaticModel() {};

		virtual bool Init(CommandList* pCommandList, const InputLayout* pInputLayout,
			const SDKMesh::sptr& mesh, const ShaderLib::sptr& shaderLib,
			const Graphics::PipelineLib::sptr& pipelineLib,
			const PipelineLayoutLib::sptr& pipelineLayoutLib,
			const DescriptorTableLib::sptr& descriptorTableLib,
			std::vector<Resource::uptr>& uploaders,
			const Format* rtvFormats = nullptr, uint32_t numRTVs = 0,
			Format dsvFormat = Format::UNKNOWN, Format shadowFormat = Format::UNKNOWN,
			bool twoSidedAll = false, bool useZEqual = true) = 0;
		virtual bool CreateDescriptorTables() = 0;

		virtual void Update(uint8_t frameIndex, DirectX::FXMMATRIX* pWorld = nullptr, bool isTemporal = true) = 0;
		virtual void Render(const CommandList* pCommandList, uint32_t mesh, PipelineLayoutIndex layout,
			SubsetFlags subsetMask = SUBSET_FULL, const DescriptorTable* pCbvPerFrameTable = nullptr,
			uint32_t numInstances = 1) = 0;

		virtual const SDKMesh::sptr& GetMesh() const = 0;

		static SDKMesh::sptr LoadSDKMesh(const Device* pDevice, const std::wstring& meshFileName,
			const TextureLib& textureLib, API api = API::DIRECTX_12);

		using uptr = std::unique_ptr<StaticModel>;
		using sptr = std::shared_ptr<StaticModel>;

		static uptr MakeUnique(const wchar_t* name = nullptr, API api = API::DIRECTX_12);
		static sptr MakeShared(const wchar_t* name = nullptr, API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Octree
	//--------------------------------------------------------------------------------------
	class OctNode;
	using Octree = std::unique_ptr<OctNode>;
	class XUSG_INTERFACE OctNode
	{
	public:
		enum Visibility : uint8_t
		{
			VISIBILITY_INTERSECT,
			VISIBILITY_INSIDE,
			VISIBILITY_OUTSIDE
		};

		//OctNode(uint32_t numModels, const StaticModel::sptr* pModels, const DirectX::XMFLOAT3& eyePt);
		virtual ~OctNode() {};

		virtual void CreateTree(SubsetFlags subsetFlag) = 0;
		virtual void Init(const DirectX::XMFLOAT3& center, float diameter) = 0;
		virtual void Sort(std::vector<DirectX::XMUINT2>& meshIDQueue,
			DirectX::CXMMATRIX viewProj, bool isNearToFar, bool isAllVisible = false) const = 0;
		virtual void TraverseBoundary(DirectX::CXMMATRIX viewProj, bool isNearToFar,
			bool allVisible, uint8_t begin, uint8_t logSize) const = 0;	// 0x, 1y, 2z
		virtual void RenderBoundary(DirectX::CXMMATRIX viewProj, bool isNearToFar, bool isAllVisible) const = 0;

		static void SetLooseCoeff(float looseCoeff);
		static void Sort(const Octree* pOctTrees, std::vector<DirectX::XMUINT2>* pMeshIDQueues,
			uint8_t opaqueQueue, uint8_t alphaQueue, DirectX::CXMMATRIX viewProj,
			DirectX::CXMMATRIX viewProjI, bool isAlphaQueueN2F);
		static void ComputeFrustumBound(DirectX::CXMMATRIX viewProjI);

		using uptr = std::unique_ptr<OctNode>;
		using sptr = std::shared_ptr<OctNode>;

		static uptr MakeUnique(uint32_t numModels, const StaticModel::sptr* pModels, const DirectX::XMFLOAT3& eyePt);
		static sptr MakeShared(uint32_t numModels, const StaticModel::sptr* pModels, const DirectX::XMFLOAT3& eyePt);
	};

	//--------------------------------------------------------------------------------------
	// Shadow
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE Shadow
	{
	public:
		//Shadow();
		virtual ~Shadow() {};

		// This runs when the application is initialized.
		virtual bool Init(const Device* pDevice, float sceneMapSize, float shadowMapSize,
			const DescriptorTableLib::sptr& descriptorTableLib,
			Format format = Format::D24_UNORM_S8_UINT,
			uint8_t numCasLevels = XUSG_NUM_CASCADE) = 0;
		virtual bool CreateDescriptorTables() = 0;

		// This runs per frame. This data could be cached when the cameras do not move.
		virtual void Update(uint8_t frameIndex, const DirectX::XMFLOAT4X4& view,
			const DirectX::XMFLOAT4X4& proj, const DirectX::XMFLOAT4& lightPt) = 0;

		virtual void SetViewport(const CommandList* pCommandList, uint8_t i) = 0;
		virtual void GetShadowMatrices(DirectX::XMMATRIX* pShadows) const = 0;

		virtual const DepthStencil::uptr& GetShadowMap() const = 0;
		virtual const DescriptorTable& GetShadowDescriptorTable() const = 0;
		virtual const Framebuffer& GetFramebuffer() const = 0;
		virtual DirectX::FXMMATRIX GetShadowMatrix(uint8_t i) const = 0;
		virtual DirectX::FXMMATRIX GetShadowViewMatrix() const = 0;

		using uptr = std::unique_ptr<Shadow>;
		using sptr = std::shared_ptr<Shadow>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Spherical harmonics
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE SphericalHarmonics
	{
	public:
		//SphericalHarmonics();
		virtual ~SphericalHarmonics() {}

		virtual bool Init(const Device* pDevice, const ShaderLib::sptr& shaderLib,
			const Compute::PipelineLib::sptr& computePipelineLib,
			const PipelineLayoutLib::sptr& pipelineLayoutLib,
			const DescriptorTableLib::sptr& descriptorTableLib,
			uint8_t baseCSIndex, uint8_t descriptorHeapIndex = 0) = 0;

		virtual void Transform(CommandList* pCommandList, Resource* pRadiance,
			const DescriptorTable& srvTable, uint8_t order = 3) = 0;

		virtual StructuredBuffer::sptr GetSHCoefficients() const = 0;

		virtual const DescriptorTable& GetSHCoeffSRVTable() const = 0;

		using uptr = std::unique_ptr<SphericalHarmonics>;
		using sptr = std::shared_ptr<SphericalHarmonics>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Nature objects 
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE Nature
	{
	public:
		//Nature();
		virtual ~Nature() {};

		virtual bool Init(CommandList* pCommandList, const ShaderLib::sptr& shaderLib,
			const Graphics::PipelineLib::sptr& graphicsPipelineLib,
			const Compute::PipelineLib::sptr& computePipelineLib,
			const PipelineLayoutLib::sptr& pipelineLayoutLib,
			const DescriptorTableLib::sptr& descriptorTableLib,
			const std::wstring& skyTexture, std::vector<Resource::uptr>& uploaders,
			bool renderWater, Format rtvFormat = Format::R11G11B10_FLOAT,
			Format dsvFormat = Format::D24_UNORM_S8_UINT) = 0;
		virtual bool CreateResources(const Device* pDevice, const ShaderResource::sptr& sceneColor,
			const DepthStencil* pDepth, bool renderWater) = 0;

		virtual void Update(uint8_t frameIndex, DirectX::FXMMATRIX* pViewProj, DirectX::FXMMATRIX* pWorld = nullptr) = 0;
		virtual void SetGlobalCBVTables(DescriptorTable cbvImmutable, DescriptorTable cbvPerFrameTable) = 0;
		virtual void RenderSky(const CommandList* pCommandList) = 0;
		virtual void RenderWater(CommandList* pCommandList, const Framebuffer& framebuffer,
			uint32_t& numBarriers, ResourceBarrier* pBarriers) = 0;

		virtual Descriptor GetSkySRV() const = 0;
		virtual DescriptorTable GetSHCoeffSRVTable(CommandList* pCommandList) = 0;

		using uptr = std::unique_ptr<Nature>;
		using sptr = std::shared_ptr<Nature>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Scene
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE Scene
	{
	public:
		enum GBufferIndex : uint8_t
		{
			ALBEDO_IDX,
			NORMAL_IDX,
			RGHMTL_IDX,
#if XUSG_TEMPORAL
			MOTION_IDX,
#endif
			AO_IDX,

			NUM_GBUFFER,
			NUM_GB_FIXED = RGHMTL_IDX + 1,
			NUM_GB_RTV = AO_IDX
		};

		enum CBVTableIndex : uint8_t
		{
			CBV_IMMUTABLE,
			CBV_PER_FRAME_VS,
			CBV_PER_FRAME_PS = CBV_PER_FRAME_VS + XUSG_FRAME_COUNT,	// Window size dependent
#if XUSG_TEMPORAL_AA
			CBV_TEMPORAL_BIAS0 = CBV_PER_FRAME_PS + XUSG_FRAME_COUNT,
			CBV_TEMPORAL_BIAS,

			NUM_CBV_TABLE = CBV_TEMPORAL_BIAS + XUSG_FRAME_COUNT
#else
			NUM_CBV_TABLE = CBV_PER_FRAME_PS + FrameCount
#endif
		};

		//Scene();
		virtual ~Scene() {};

		virtual bool LoadAssets(void* pSceneReader, CommandList* pCommandList,
			const ShaderLib::sptr& shaderLib,
			const Graphics::PipelineLib::sptr& graphicsPipelineLib,
			const Compute::PipelineLib::sptr& computePipelineLib,
			const PipelineLayoutLib::sptr& pipelineLayoutLib,
			const DescriptorTableLib::sptr& descriptorTableLib,
			std::vector<Resource::uptr>& uploaders, Format rtvFormat,
			Format dsvFormat, Format shadowFormat = Format::D24_UNORM_S8_UINT,
			bool useIBL = false) = 0;
		virtual bool ChangeWindowSize(CommandList* pCommandList,
			std::vector<Resource::uptr>& uploaders,
			const RenderTarget::sptr& sceneColor,
			const DepthStencil::sptr& sceneDepth,
			const RenderTarget::sptr& sceneShade) = 0;
		virtual bool CreateResources(CommandList* pCommandList, std::vector<Resource::uptr>& uploaders) = 0;

		virtual void Update(uint8_t frameIndex, double time, float timeStep) = 0;
		virtual void Update(uint8_t frameIndex, double time, float timeStep, DirectX::CXMMATRIX view,
			DirectX::CXMMATRIX proj, DirectX::CXMVECTOR eyePt, bool enableJitter = true) = 0;
		virtual void Render(CommandList* pCommandList) = 0;
		virtual void SkinCharacters(CommandList* pCommandList, uint32_t& numBarriers,
			ResourceBarrier* pBarriers, bool reset = false) = 0;
		virtual void RenderDepth(CommandList* pCommandList, uint32_t& numBarriers,
			ResourceBarrier* pBarriers, BarrierFlag barrierFlag) = 0;
		virtual void RenderShadowMap(CommandList* pCommandList, uint32_t& numBarriers,
			ResourceBarrier* pBarriers, BarrierFlag barrierFlag) = 0;
		virtual void RenderGBuffersOpaque(CommandList* pCommandList, uint32_t& numBarriers,
			ResourceBarrier* pBarriers, const BarrierFlag barrierFlags[NUM_GB_FIXED]) = 0;
		virtual void RenderGBuffersAlpha(CommandList* pCommandList, uint32_t& numBarriers,
			ResourceBarrier* pBarriers, const BarrierFlag barrierFlags[NUM_GB_RTV]) = 0;
		virtual void RenderGBuffers(CommandList* pCommandList, uint32_t& numBarriers,
			ResourceBarrier* pBarriers, const BarrierFlag barrierFlags[NUM_GB_RTV]) = 0;
		virtual void SetViewProjMatrix(DirectX::CXMMATRIX view, DirectX::CXMMATRIX proj, bool enableJitter) = 0;
		virtual void SetEyePoint(DirectX::CXMVECTOR eyePt) = 0;
		virtual void SetFocusAndDistance(DirectX::CXMVECTOR focus_dist) = 0;
		virtual void SetRenderTarget(const RenderTarget::sptr& sceneColor, const DepthStencil::sptr& sceneDepth,
			const RenderTarget::sptr& sceneShade, bool createFramebuffer = true) = 0;
		virtual void SetViewport(const Viewport& viewport, const RectRange& scissorRect) = 0;

		virtual DirectX::FXMVECTOR GetFocusAndDistance() const = 0;
		virtual const DescriptorTable& GetCBVTable(uint8_t i) const = 0;
		virtual const RenderTarget* GetGBuffer(uint8_t i) const = 0;

		virtual std::vector<Character::uptr>& GetCharacters() = 0;
		virtual std::vector<StaticModel::sptr>& GetStaticModels() = 0;

		using uptr = std::unique_ptr<Scene>;
		using sptr = std::shared_ptr<Scene>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Postprocess
	//--------------------------------------------------------------------------------------
	class XUSG_INTERFACE Postprocess
	{
	public:
		enum PipelineIndex : uint8_t
		{
			ANTIALIAS,
			POST_EFFECTS,
			RESAMPLE_LUM,
			LUM_ADAPT,
			TONE_MAP,
			UNSHARP,

			NUM_PIPELINE
		};

		//Postprocess();
		virtual ~Postprocess() {};

		virtual bool Init(const Device* pDevice, const ShaderLib::sptr& shaderLib,
			const Graphics::PipelineLib::sptr& graphicsPipelineLib,
			const Compute::PipelineLib::sptr& computePipelineLib,
			const PipelineLayoutLib::sptr& pipelineLayoutLib,
			const DescriptorTableLib::sptr& descriptorTableLib,
			Format hdrFormat, Format ldrFormat) = 0;
		virtual bool ChangeWindowSize(const Device* pDevice, const Texture* pReference) = 0;

		virtual void Update(const DescriptorTable& cbvImmutable, const DescriptorTable& cbvPerFrameTable,
			float timeStep) = 0;
		virtual void Render(CommandList* pCommandList, RenderTarget* pDst, Texture* pSrc,
			const DescriptorTable& srvTable, bool clearRT = false) = 0;
		virtual void ScreenRender(const CommandList* pCommandList, PipelineIndex pipelineIndex,
			const DescriptorTable& srvTable, bool hasImmutableCB, bool hasPerFrameCB) = 0;
		virtual void LumAdaption(const CommandList* pCommandList, const DescriptorTable& uavSrvTable) = 0;
		virtual void Antialias(CommandList* pCommandList, RenderTarget** ppDsts, Texture** ppSrcs,
			const DescriptorTable& srvTable, uint8_t numRTVs, uint8_t numSRVs) = 0;
		virtual void Unsharp(const CommandList* pCommandList, const Descriptor* pRTVs,
			const DescriptorTable& srvTable, uint8_t numRTVs = 1) = 0;

		virtual DescriptorTable CreateTAASrvTable(const Descriptor& srvCurrent, const Descriptor& srvPrevious,
			const Descriptor& srvVelocity, const Descriptor& srvShadeAmt, const Descriptor& srvMeta) = 0;

		using uptr = std::unique_ptr<Postprocess>;
		using sptr = std::shared_ptr<Postprocess>;

		static uptr MakeUnique(API api = API::DIRECTX_12);
		static sptr MakeShared(API api = API::DIRECTX_12);
	};

	//--------------------------------------------------------------------------------------
	// Halton sequence helpers
	//--------------------------------------------------------------------------------------

	XUSG_INTERFACE float Halton(uint32_t i, uint32_t b);
	XUSG_INTERFACE DirectX::XMFLOAT2 Halton(uint32_t i);
	XUSG_INTERFACE const DirectX::XMFLOAT2& IncrementalHalton();

	//--------------------------------------------------------------------------------------
	// Intrinsic shader Ids
	//--------------------------------------------------------------------------------------

	// Vertex shaders
	enum VertexShader : uint8_t
	{
		VS_SCREEN_QUAD,

		VS_BASE_PASS,
		VS_BASE_PASS_STATIC,
		VS_DEPTH,
		VS_DEPTH_STATIC,
		VS_SHADOW,
		VS_SHADOW_STATIC,
		VS_SKINNING,

		VS_WATER
	};

	// Pixel shaders
	enum PixelShader : uint8_t
	{
		PS_DEFERRED_SHADE,
		PS_AMBIENT_OCCLUSION,

		PS_BASE_PASS,
		PS_DEPTH,
		PS_ALPHA_TEST,

		PS_SKYDOME,
		PS_SS_REFLECT,
		PS_WATER,
		PS_BLIT_2D,

		PS_POST_PROC,
		PS_TONE_MAP,
		PS_TEMPORAL_AA,

		PS_NULL_INDEX
	};

	// Compute shaders
	enum ComputeShader : uint8_t
	{
		CS_SKINNING,
		CS_SH_CUBE_MAP,
		CS_SH_SUM,
		CS_SH_NORMALIZE,
		//CS_BLIT_2D,
		CS_LUM_ADAPT
	};
}

#endif
