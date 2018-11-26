#include "Demo.h"
#include "DirectX12Headers.h"
#include "Application.h"
#include "CommandQueue.h"
#include "Helpers.h"
#include "Window.h"
#include "Libs\imgui-master\examples\directx12_example\imgui_impl_dx12.h"
#include <iostream>

#include <wrl.h>
using namespace Microsoft::WRL;
#include "DirectX12Headers.h"
#include <d3dcompiler.h>

#define STB_IMAGE_IMPLEMENTATION
#include "Libs\stb-master\stb_image.h"

#include <vector>

#include <Libs\Assimp\Include\assimp\scene.h>
#include <Libs\Assimp\Include\assimp\Importer.hpp>
#include <Libs\Assimp\Include\assimp\postprocess.h>

#include <algorithm> // For std::min and std::max.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#define ENABLE_IMGUI	0

using namespace DirectX;

// Clamp a value between a min and max range.
template<typename T>
constexpr const T& clamp(const T& val, const T& min, const T& max)
{
	return val < min ? min : val > max ? max : val;
}

struct ConstantBuffer
{
	XMMATRIX model;
	XMMATRIX view;
	XMMATRIX projection;
	XMFLOAT4 camPos;
};

static ConstantBuffer g_CBuffer;

static XMFLOAT3 g_VerticesPos[5800];

static XMFLOAT3 g_VertNormals[5800];

static XMFLOAT3 g_VertTangents[5800];

static XMFLOAT3 g_VertBitangents[5800];

static XMFLOAT2 g_VerticesTexCoords[3000];

static uint32_t g_Indices[5800];

static uint32_t g_NumberOfIndices = 0u;

bool g_CapturingMouse = false;

Demo::Demo(const std::wstring& name, int width, int height, bool vSync)
	: super(name, width, height, vSync)
	, m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
	, m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
	, m_ContentLoaded(false)
{
}

void Demo::UpdateBufferResource(
	ComPtr<ID3D12GraphicsCommandList2> commandList,
	ID3D12Resource** pDestinationResource,
	ID3D12Resource** pIntermediateResource,
	size_t numElements, size_t elementSize, const void* bufferData,
	D3D12_RESOURCE_FLAGS flags)
{
	auto device = Application::Get().GetDevice();

	size_t bufferSize = numElements * elementSize;

	// Create a committed resource for the GPU resource in a default heap.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pDestinationResource)));

	// Create an committed resource for the upload.
	if (bufferData)
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pIntermediateResource)));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(commandList.Get(),
			*pDestinationResource, *pIntermediateResource,
			0, 0, 1, &subresourceData);
	}
}

void Demo::UpdateTextureResource(
	ComPtr<ID3D12GraphicsCommandList2> commandList,
	ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
	size_t width, size_t height, size_t mipLevels, size_t channels, unsigned char* textureData,
	D3D12_RESOURCE_FLAGS flags)
{
	auto device = Application::Get().GetDevice();

	CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, mipLevels, 1U, 0U, flags);

	// Create a committed resource for the GPU resource in a default heap.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pDestinationResource)));

	// Create an committed resource for the upload.
	if (textureData)
	{
		uint64_t textureUploadBufferSize;
		device->GetCopyableFootprints(&textureDesc, 0, mipLevels, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pIntermediateResource)));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = textureData;
		subresourceData.RowPitch = width * channels;
		subresourceData.SlicePitch = subresourceData.RowPitch * height;

		UpdateSubresources(commandList.Get(),
			*pDestinationResource, *pIntermediateResource,
			0, 0, 1, &subresourceData);
	}
}


bool Demo::LoadContent()
{
	//Create the camera
	//m_Camera = new Camera(GetClientWidth(), GetClientHeight(), 1000.0f, 0.1f);
	//m_Camera->Update();

	m_Camera = new Camera(XMFLOAT3(0.0f, 0.0f, -100.0f));
	m_LastX = GetClientWidth() / 2.0f;
	m_LastY = GetClientHeight() / 2.0f;
	m_FirstMouse = true;
	m_DeltaTime = 0.0f;

	const float screenAspect = (float)GetClientWidth() / (float)GetClientHeight();

	DirectX::XMMATRIX projection = XMMatrixPerspectiveFovLH(m_Camera->Zoom, screenAspect, 1000.0f, 0.1f);
	m_Camera->SetProjectionMatrix(projection);

	//Create Sphere Mesh
	LoadSphere();

	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	auto commandQueueCompute = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	auto commandListCompute = commandQueueCompute->GetCommandList();

	auto commandQueueDirect = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandListDirect = commandQueueDirect->GetCommandList();

	commandQueueCompute = commandQueueDirect;
	commandListCompute = commandListDirect;

#pragma region GEOMETRY_BUFFERS_CREATION_UPLOAD

	// Upload vertex buffer position data.
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateVertexData;
	UpdateBufferResource(commandList,
		&m_VertexBuffers[VBE_Position], &intermediateVertexData,
		m_NumSphereVertices, sizeof(XMFLOAT3), g_VerticesPos);

	// Create the vertex buffer position view.
	m_VertexBufferViews[VBE_Position].BufferLocation = m_VertexBuffers[VBE_Position]->GetGPUVirtualAddress();
	m_VertexBufferViews[VBE_Position].SizeInBytes = m_NumSphereVertices * sizeof(XMFLOAT3);
	m_VertexBufferViews[VBE_Position].StrideInBytes = sizeof(XMFLOAT3);

	m_VertexBuffers[VBE_Position]->SetName(L"VBE_Position Buffer");

	// Upload vertex buffer texcoords data.
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateTexCoordData;
	UpdateBufferResource(commandList,
		&m_VertexBuffers[VBE_TexCoord], &intermediateTexCoordData,
		m_NumSphereVertices, sizeof(XMFLOAT2), g_VerticesTexCoords);

	// Create the vertex buffer texcoords view.
	m_VertexBufferViews[VBE_TexCoord].BufferLocation = m_VertexBuffers[VBE_TexCoord]->GetGPUVirtualAddress();
	m_VertexBufferViews[VBE_TexCoord].SizeInBytes = m_NumSphereVertices * sizeof(XMFLOAT2);
	m_VertexBufferViews[VBE_TexCoord].StrideInBytes = sizeof(XMFLOAT2);

	m_VertexBuffers[VBE_TexCoord]->SetName(L"VBE_TexCoord Buffer");

	// Upload vertex buffer normal data.
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateNormalsData;
	UpdateBufferResource(commandList,
		&m_VertexBuffers[VBE_Normals], &intermediateNormalsData,
		m_NumSphereVertices, sizeof(XMFLOAT3), g_VertNormals);

	// Create the vertex buffer normal view.
	m_VertexBufferViews[VBE_Normals].BufferLocation = m_VertexBuffers[VBE_Normals]->GetGPUVirtualAddress();
	m_VertexBufferViews[VBE_Normals].SizeInBytes = m_NumSphereVertices * sizeof(XMFLOAT3);
	m_VertexBufferViews[VBE_Normals].StrideInBytes = sizeof(XMFLOAT3);

	m_VertexBuffers[VBE_Normals]->SetName(L"VBE_Normals Buffer");

	// Upload vertex buffer tangent data.
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateTangentsData;
	UpdateBufferResource(commandList,
		&m_VertexBuffers[VBE_Tangents], &intermediateTangentsData,
		m_NumSphereVertices, sizeof(XMFLOAT3), g_VertTangents);

	// Create the vertex buffer tangent view.
	m_VertexBufferViews[VBE_Tangents].BufferLocation = m_VertexBuffers[VBE_Tangents]->GetGPUVirtualAddress();
	m_VertexBufferViews[VBE_Tangents].SizeInBytes = m_NumSphereVertices * sizeof(XMFLOAT3);
	m_VertexBufferViews[VBE_Tangents].StrideInBytes = sizeof(XMFLOAT3);

	m_VertexBuffers[VBE_Tangents]->SetName(L"VBE_Tangents Buffer");

	// Upload vertex buffer bitangent data.
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateBitangentData;
	UpdateBufferResource(commandList,
		&m_VertexBuffers[VBE_Bitangents], &intermediateBitangentData,
		m_NumSphereVertices, sizeof(XMFLOAT3), g_VertBitangents);

	// Create the vertex buffer position view.
	m_VertexBufferViews[VBE_Bitangents].BufferLocation = m_VertexBuffers[VBE_Bitangents]->GetGPUVirtualAddress();
	m_VertexBufferViews[VBE_Bitangents].SizeInBytes = m_NumSphereVertices * sizeof(XMFLOAT3);
	m_VertexBufferViews[VBE_Bitangents].StrideInBytes = sizeof(XMFLOAT3);

	m_VertexBuffers[VBE_Bitangents]->SetName(L"VBE_Bitangents Buffer");

	// Upload index buffer data.
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateIndexData;
	UpdateBufferResource(commandList,
		&m_IndexBuffer, &intermediateIndexData,
		m_NumSphereFaces * 3, sizeof(uint32_t), g_Indices);

	// Create index buffer view.
	m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_IndexBufferView.SizeInBytes = m_NumSphereFaces * 3 * sizeof(uint32_t);

	m_IndexBuffer->SetName(L"Index Buffer");

#pragma endregion

#pragma region CONSTANT_BUFFER_CREATION

	// Constant buffer on upload heap
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_ConstantBufferUploadHeap)));

	m_ConstantBufferUploadHeap->SetName(L"Constant Buffer");

	ZeroMemory(&g_CBuffer, sizeof(ConstantBuffer));

	CD3DX12_RANGE readRange(0, 0);

	ThrowIfFailed(m_ConstantBufferUploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&m_ConstantBufferGPUAddress[0])));

	memcpy(m_ConstantBufferGPUAddress[0], &g_CBuffer, sizeof(ConstantBuffer));

#pragma endregion

#pragma region DESCRIPTOR_HEAPS_CREATION

	// Create the descriptor heap for the depth-stencil view.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));

	// Create the descriptor heap for the render target view.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 1;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_HDRHeap)));

	m_HDRHandleIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC mainHeapDesc = {};
	mainHeapDesc.NumDescriptors = 4;
	mainHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	mainHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&mainHeapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC imguiHeapDesc = {};
	imguiHeapDesc.NumDescriptors = 1;
	imguiHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	imguiHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&imguiHeapDesc, IID_PPV_ARGS(&m_ImguiHeap)));

#pragma endregion

#pragma region PBR_TEXTURES_LOADING

	// Load Textures
	int w_al, h_al, comp_al;
	unsigned char* albedo = stbi_load("Resources/Textures/MaterialAlbedo.png", &w_al, &h_al, &comp_al, STBI_rgb_alpha);

	uint32_t mipLvl_al = static_cast<uint32_t>(std::floor(std::log2(std::max(w_al, h_al)))) + 1;

	int w_nr, h_nr, comp_nr;
	unsigned char* normal = stbi_load("Resources/Textures/MaterialNormal.png", &w_nr, &h_nr, &comp_nr, STBI_rgb_alpha);

	uint32_t mipLvl_nr = static_cast<uint32_t>(std::floor(std::log2(std::max(w_nr, h_nr)))) + 1;

	int w_rm, h_rm, comp_rm;
	unsigned char* roughnessMetallic = stbi_load("Resources/Textures/MaterialMetallicRoughness.png", &w_rm, &h_rm, &comp_rm, STBI_rgb_alpha);

	uint32_t mipLvl_rm = static_cast<uint32_t>(std::floor(std::log2(std::max(w_rm, h_rm)))) + 1;

	//Create texture resources
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	m_ResourcesHeapIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//ALBEDO TEXTURE
	ComPtr<ID3D12Resource> intermediateTextureResource;
	UpdateTextureResource(commandList,
		&m_TextureResource[PBR_Albedo], &intermediateTextureResource,
		w_al, h_al, mipLvl_al, comp_al, albedo, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	m_TextureResource[PBR_Albedo]->SetName(L"Albedo Texture Resource");

	TransitionResource(commandListDirect, m_TextureResource[PBR_Albedo], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	memset(&m_TextureResourceView[PBR_Albedo], 0, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	m_TextureResourceView[PBR_Albedo].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	m_TextureResourceView[PBR_Albedo].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_TextureResourceView[PBR_Albedo].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	m_TextureResourceView[PBR_Albedo].Texture2D.MipLevels = mipLvl_al;

	device->CreateShaderResourceView(m_TextureResource[PBR_Albedo].Get(), &m_TextureResourceView[PBR_Albedo], heapHandle);

	heapHandle.Offset(m_ResourcesHeapIncrement);

	//NORMAL TEXTURE
	ComPtr<ID3D12Resource> intermediateNormalTexture;
	UpdateTextureResource(commandList,
		&m_TextureResource[PBR_Normal], &intermediateNormalTexture,
		w_nr, h_nr, mipLvl_nr, comp_nr, normal, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	m_TextureResource[PBR_Normal]->SetName(L"Normal Texture Resource");

	TransitionResource(commandListDirect, m_TextureResource[PBR_Normal], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	memset(&m_TextureResourceView[PBR_Normal], 0, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	m_TextureResourceView[PBR_Normal].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	m_TextureResourceView[PBR_Normal].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_TextureResourceView[PBR_Normal].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	m_TextureResourceView[PBR_Normal].Texture2D.MipLevels = mipLvl_nr;

	device->CreateShaderResourceView(m_TextureResource[PBR_Normal].Get(), &m_TextureResourceView[PBR_Normal], heapHandle);

	heapHandle.Offset(m_ResourcesHeapIncrement);

	//ROUGHNESS-METALLIC TEXTURE
	ComPtr<ID3D12Resource> intermediateRoughnessMetallic;
	UpdateTextureResource(commandList,
		&m_TextureResource[PBR_MetallicRoughness], &intermediateRoughnessMetallic,
		w_rm, h_rm, mipLvl_rm, comp_rm, roughnessMetallic, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	m_TextureResource[PBR_MetallicRoughness]->SetName(L"MetRough Texture Resource");

	TransitionResource(commandListDirect, m_TextureResource[PBR_MetallicRoughness], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	memset(&m_TextureResourceView[PBR_MetallicRoughness], 0, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	m_TextureResourceView[PBR_MetallicRoughness].Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	m_TextureResourceView[PBR_MetallicRoughness].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_TextureResourceView[PBR_MetallicRoughness].ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	m_TextureResourceView[PBR_MetallicRoughness].Texture2D.MipLevels = mipLvl_rm;

	device->CreateShaderResourceView(m_TextureResource[PBR_MetallicRoughness].Get(), &m_TextureResourceView[PBR_MetallicRoughness], heapHandle);

#pragma endregion

	// create a static sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

#pragma region LOADING_SHADING_PASS_SHADERS

	// Load the vertex shader.
	ComPtr<ID3DBlob> shadingPassVertexBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"ShadingPassVertex.cso", &shadingPassVertexBlob));

	// Load the pixel shader.
	ComPtr<ID3DBlob> shadingPassPixelBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"ShadingPassPixel.cso", &shadingPassPixelBlob));

	// Create the vertex input layout
	D3D12_INPUT_ELEMENT_DESC shadingPassInputLayout[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 3, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 4, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Create a root signature.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureDataShadingPass = {};
	featureDataShadingPass.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureDataShadingPass, sizeof(featureDataShadingPass))))
	{
		featureDataShadingPass.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS shadingPassRootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// create a descriptor range (descriptor table) and fill it out
	// this is a range of descriptors inside a descriptor heap
	CD3DX12_DESCRIPTOR_RANGE1 DescRange[1];
	DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, PBR_Count, 0);

	//D3D12_DESCRIPTOR_RANGE1 shadingPassDescriptorTableRanges[1]; // only one range right now
	//shadingPassDescriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // this is a range of shader resource views (descriptors)
	//shadingPassDescriptorTableRanges[0].NumDescriptors = PBR_Count; // we have three textures right now
	//shadingPassDescriptorTableRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
	//shadingPassDescriptorTableRanges[0].RegisterSpace = 0; // space 0. can usually be zero
	//shadingPassDescriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables
	//shadingPassDescriptorTableRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	CD3DX12_ROOT_PARAMETER1 shadingPassRootParameters[2];
	shadingPassRootParameters[0].InitAsConstantBufferView(0u, 0u, D3D12_ROOT_DESCRIPTOR_FLAG_NONE);
	shadingPassRootParameters[1].InitAsDescriptorTable(1u, &DescRange[0], D3D12_SHADER_VISIBILITY_PIXEL);


	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC shadingPassRootSignatureDescription;
	shadingPassRootSignatureDescription.Init_1_1(_countof(shadingPassRootParameters), shadingPassRootParameters, 1u, &sampler, shadingPassRootSignatureFlags);

	// Serialize the root signature.
	ComPtr<ID3DBlob> shadingPassRootSignatureBlob;
	ComPtr<ID3DBlob> shadingPassErrorBlob;
	HRESULT hr = D3DX12SerializeVersionedRootSignature(&shadingPassRootSignatureDescription,
		featureDataShadingPass.HighestVersion, &shadingPassRootSignatureBlob, &shadingPassErrorBlob);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)shadingPassErrorBlob->GetBufferPointer());
		return false;
	}

	// Create the root signature.
	ThrowIfFailed(device->CreateRootSignature(0, shadingPassRootSignatureBlob->GetBufferPointer(),
		shadingPassRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignatures[PipelineStage_ShadingPass])));

#pragma endregion

#pragma region LOADING_TONEMAPPING_SHADERS

	// Load the vertex shader.
	ComPtr<ID3DBlob> tonemappingPassVertexBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"ToneMappingVertex.cso", &tonemappingPassVertexBlob));

	// Load the pixel shader.
	ComPtr<ID3DBlob> tonemappingPassPixelBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"ToneMappingPixel.cso", &tonemappingPassPixelBlob));

	// Create the vertex input layout
	D3D12_INPUT_ELEMENT_DESC toneMappingInputLayout[] = {
		{ "SV_VertexID", 0, DXGI_FORMAT_R32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Create a root signature.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE toneMappingFeaturedData = {};
	toneMappingFeaturedData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &toneMappingFeaturedData, sizeof(toneMappingFeaturedData))))
	{
		toneMappingFeaturedData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS toneMappingRootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// create a descriptor range (descriptor table) and fill it out
	// this is a range of descriptors inside a descriptor heap
	D3D12_DESCRIPTOR_RANGE1 toneMappingDescriptorTableRanges[1]; // only one range right now
	toneMappingDescriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // this is a range of shader resource views (descriptors)
	toneMappingDescriptorTableRanges[0].NumDescriptors = 1; // we only have one texture right now, so the range is only 1
	toneMappingDescriptorTableRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
	toneMappingDescriptorTableRanges[0].RegisterSpace = 0; // space 0. can usually be zero
	toneMappingDescriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables
	toneMappingDescriptorTableRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;


	CD3DX12_ROOT_PARAMETER1 toneMappingRootParameters[1];
	toneMappingRootParameters[0].InitAsDescriptorTable(1u, &toneMappingDescriptorTableRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);


	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC toneMappingRootSignatureDescription;
	toneMappingRootSignatureDescription.Init_1_1(_countof(toneMappingRootParameters), toneMappingRootParameters, 1u, &sampler, toneMappingRootSignatureFlags);

	// Serialize the root signature.
	ComPtr<ID3DBlob> toneMappingRootSignatureBlob;
	ComPtr<ID3DBlob> toneMappingErrorBlob;
	hr = D3DX12SerializeVersionedRootSignature(&toneMappingRootSignatureDescription,
		toneMappingFeaturedData.HighestVersion, &toneMappingRootSignatureBlob, &toneMappingErrorBlob);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)toneMappingErrorBlob->GetBufferPointer());
		return false;
	}

	// Create the root signature.
	ThrowIfFailed(device->CreateRootSignature(0, toneMappingRootSignatureBlob->GetBufferPointer(),
		toneMappingRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignatures[PipelineStage_TonemapPass])));

#pragma endregion

#pragma region PSO_CREATIONS

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RastState;
		CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendState;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthState;
	} pipelineStateStream;

	CD3DX12_RASTERIZER_DESC rastDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rastDesc.CullMode = D3D12_CULL_MODE_BACK;

	CD3DX12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	blendDesc.RenderTarget[0].BlendEnable = false;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	CD3DX12_DEPTH_STENCIL_DESC depthDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	depthDesc.DepthEnable = false;
	depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	depthDesc.StencilEnable = false;


	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature = m_RootSignatures[PipelineStage_TonemapPass].Get();
	pipelineStateStream.InputLayout = { toneMappingInputLayout, _countof(toneMappingInputLayout) };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(tonemappingPassVertexBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(tonemappingPassPixelBlob.Get());
	pipelineStateStream.RTVFormats = rtvFormats;
	pipelineStateStream.RastState = rastDesc;
	pipelineStateStream.BlendState = blendDesc;
	pipelineStateStream.DepthState = depthDesc;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineStateStream
	};
	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineStates[PipelineStage_TonemapPass])));


	depthDesc.DepthEnable = true;
	depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	rtvFormats.RTFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;

	pipelineStateStream.pRootSignature = m_RootSignatures[PipelineStage_ShadingPass].Get();
	pipelineStateStream.InputLayout = { shadingPassInputLayout, _countof(shadingPassInputLayout) };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(shadingPassVertexBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(shadingPassPixelBlob.Get());
	pipelineStateStream.RTVFormats = rtvFormats;
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RastState = rastDesc;
	pipelineStateStream.BlendState = blendDesc;
	pipelineStateStream.DepthState = depthDesc;

	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineStates[PipelineStage_ShadingPass])));

#pragma endregion

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

#pragma region PBR_TEXTURES_MIPMAPPING

	struct DWParam
	{
		DWParam(FLOAT f) : Float(f) {}
		DWParam(UINT u) : Uint(u) {}

		void operator= (FLOAT f) { Float = f; }
		void operator= (UINT u) { Uint = u; }

		union
		{
			FLOAT Float;
			UINT Uint;
		};
	};

	//The compute shader expects 2 floats, the source texture and the destination texture
	CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
	CD3DX12_ROOT_PARAMETER rootParameters[3];
	srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

	rootParameters[0].InitAsConstants(2, 0);
	rootParameters[1].InitAsDescriptorTable(1, &srvCbvRanges[0]);
	rootParameters[2].InitAsDescriptorTable(1, &srvCbvRanges[1]);

	//Static sampler used to get the linearly interpolated color for the mipmaps
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//Create the root signature for the mipmap compute shader from the parameters and sampler above
	ID3DBlob *signature;
	ID3DBlob *error;
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	ID3D12RootSignature *mipMapRootSignature;
	device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mipMapRootSignature));

	//Create the descriptor heap with layout: source texture - destination texture
	uint32_t requiredHeapSize = (mipLvl_al - 1) + (mipLvl_nr - 1) + (mipLvl_rm - 1);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 2 * requiredHeapSize;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ID3D12DescriptorHeap *descriptorHeapCompute;
	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeapCompute));
	UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//Read the compute shader blob
	ComPtr<ID3DBlob> mipMapComputeShader;
	ThrowIfFailed(D3DReadFileToBlob(L"MipMappingComputeShader.cso", &mipMapComputeShader));

	//Create pipeline state object for the compute shader using the root signature.
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = mipMapRootSignature;
	psoDesc.CS = { reinterpret_cast<UINT8*>(mipMapComputeShader->GetBufferPointer()), mipMapComputeShader->GetBufferSize() };
	ID3D12PipelineState *psoMipMaps;
	device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&psoMipMaps));

	//Prepare the shader resource view description for the source texture
	D3D12_SHADER_RESOURCE_VIEW_DESC srcTextureSRVDesc = {};
	srcTextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srcTextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	//Prepare the unordered access view description for the destination texture
	D3D12_UNORDERED_ACCESS_VIEW_DESC destTextureUAVDesc = {};
	destTextureUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	commandListCompute->SetComputeRootSignature(mipMapRootSignature);
	commandListCompute->SetPipelineState(psoMipMaps);
	commandListCompute->SetDescriptorHeaps(1, &descriptorHeapCompute);

	//CPU handle for the first descriptor on the descriptor heap, used to fill the heap
	CD3DX12_CPU_DESCRIPTOR_HANDLE currentCPUHandle(descriptorHeapCompute->GetCPUDescriptorHandleForHeapStart(), 0, descriptorSize);

	//GPU handle for the first descriptor on the descriptor heap, used to initialize the descriptor tables
	CD3DX12_GPU_DESCRIPTOR_HANDLE currentGPUHandle(descriptorHeapCompute->GetGPUDescriptorHandleForHeapStart(), 0, descriptorSize);


	for (uint32_t i = 0; i < PBR_Count; i++)
	{
		ID3D12Resource* texture = m_TextureResource[i].Get();

		//Transition from pixel shader resource to unordered access
		commandListCompute->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		commandListCompute->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(texture));

		for (uint32_t TopMip = 0; TopMip < mipLvl_al - 1; TopMip++)
		{
			//Get mipmap dimensions
			//uint32_t dstWidth = std::max(w_al >> (TopMip + 1), 1);
			//uint32_t dstHeight = std::max(h_al >> (TopMip + 1), 1);

			uint32_t srcWidth = w_al >> TopMip;
			uint32_t srcHeight = w_al >> TopMip;
			uint32_t dstWidth = srcWidth >> 1;
			uint32_t dstHeight = srcHeight >> 1;

			//Create shader resource view for the source texture in the descriptor heap
			srcTextureSRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srcTextureSRVDesc.Texture2D.MipLevels = 1;
			srcTextureSRVDesc.Texture2D.MostDetailedMip = TopMip;
			device->CreateShaderResourceView(texture, &srcTextureSRVDesc, currentCPUHandle);
			currentCPUHandle.Offset(1, descriptorSize);

			//Create unordered access view for the destination texture in the descriptor heap
			destTextureUAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			destTextureUAVDesc.Texture2D.MipSlice = TopMip + 1;
			device->CreateUnorderedAccessView(texture, nullptr, &destTextureUAVDesc, currentCPUHandle);
			currentCPUHandle.Offset(1, descriptorSize);

			//Pass the destination texture pixel size to the shader as constants
			commandListCompute->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstWidth).Uint, 0);
			commandListCompute->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstHeight).Uint, 1);

			//Pass the source and destination texture views to the shader via descriptor tables
			commandListCompute->SetComputeRootDescriptorTable(1, currentGPUHandle);
			currentGPUHandle.Offset(1, descriptorSize);
			commandListCompute->SetComputeRootDescriptorTable(2, currentGPUHandle);
			currentGPUHandle.Offset(1, descriptorSize);

			//Dispatch the compute shader with one thread per 8x8 pixels
			commandListCompute->Dispatch(std::max(dstWidth / 8, 1u), std::max(dstHeight / 8, 1u), 1);

			//Wait for all accesses to the destination texture UAV to be finished before generating the next mipmap, as it will be the source texture for the next mipmap
			commandListCompute->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(texture));

		}

		//When done with the texture, transition it's state back to be a pixel shader resource
		commandListCompute->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

#pragma endregion

	auto fenceValue2 = commandQueueDirect->ExecuteCommandList(commandListDirect);
	commandQueueDirect->WaitForFenceValue(fenceValue2);

	m_ContentLoaded = true;
	stbi_image_free(albedo);
	stbi_image_free(normal);
	stbi_image_free(roughnessMetallic);

	// Resize/Create the depth buffer.
	ResizeDepthBuffer(GetClientWidth(), GetClientHeight());
	// Resize/Create the render target.
	ResizeRenderTarget(GetClientWidth(), GetClientHeight());


#ifdef ENABLE_IMGUI
	//Init IMGUI
	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	ImGui_ImplDX12_Init(m_pWindow->WindowHandle(), 3, Application::Get().GetDevice().Get(),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_ImguiHeap->GetCPUDescriptorHandleForHeapStart(),
		m_ImguiHeap->GetGPUDescriptorHandleForHeapStart());

	ImGui::StyleColorsDark();
	ImGui::GetIO().WantCaptureKeyboard = false;

#endif

	return true;
}

void Demo::ResizeDepthBuffer(int width, int height)
{
	if (m_ContentLoaded)
	{
		// Flush any GPU commands that might be referencing the depth buffer.
		Application::Get().Flush();

		width = std::max(1, width);
		height = std::max(1, height);

		auto device = Application::Get().GetDevice();

		// Resize screen dependent resources.
		// Create a depth buffer.
		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		optimizedClearValue.DepthStencil = { 1.0f, 0 };

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
				1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optimizedClearValue,
			IID_PPV_ARGS(&m_DepthBuffer)
		));

		m_DepthBuffer->SetName(L"Depth Buffer Resource");

		// Update the depth-stencil view.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;
		dsv.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv,
			m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
	}
}

void Demo::ResizeRenderTarget(int width, int height)
{
	if (m_ContentLoaded)
	{
		// Flush any GPU commands that might be referencing the depth buffer.
		Application::Get().Flush();

		width = std::max(1, width);
		height = std::max(1, height);

		auto device = Application::Get().GetDevice();

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height,
				1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
			D3D12_RESOURCE_STATE_PRESENT,
			nullptr,
			IID_PPV_ARGS(&m_HDRBuffer)
		));

		m_HDRBuffer->SetName(L"HDR Render Target");

		// Update the render-target view.
		D3D12_RENDER_TARGET_VIEW_DESC rtv = {};
		rtv.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtv.Texture2D.MipSlice = 0;

		device->CreateRenderTargetView(m_HDRBuffer.Get(), &rtv,
			m_HDRHeap->GetCPUDescriptorHandleForHeapStart());

		memset(&m_HDRBufferView, 0, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

		m_HDRBufferView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		m_HDRBufferView.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		m_HDRBufferView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		m_HDRBufferView.Texture2D.MipLevels = 1;

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(m_HDRHandleIncrement * PBR_Count);

		device->CreateShaderResourceView(m_HDRBuffer.Get(), &m_HDRBufferView, handle);
	}
}

void Demo::RenderImgui()
{
}

void Demo::OnResize(ResizeEventArgs& e)
{
	if (e.Width != GetClientWidth() || e.Height != GetClientHeight())
	{
		super::OnResize(e);

		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
			static_cast<float>(e.Width), static_cast<float>(e.Height));

		ResizeDepthBuffer(e.Width, e.Height);
		ResizeRenderTarget(e.Width, e.Height);
	}
}

void Demo::UnloadContent()
{
	m_ContentLoaded = false;

#ifdef ENABLE_IMGUI
	ImGui_ImplDX12_Shutdown();
	ImGui::DestroyContext();
#endif
}

void Demo::OnUpdate(UpdateEventArgs& e)
{
	static uint64_t frameCount = 0;
	static double totalTime = 0.0;

	super::OnUpdate(e);

	m_DeltaTime = e.ElapsedTime;

	totalTime += m_DeltaTime;
	frameCount++;

	if (totalTime > 1.0)
	{
		double fps = frameCount / totalTime;

		char buffer[512];
		sprintf_s(buffer, "FPS: %f\n", fps);
		OutputDebugStringA(buffer);

		frameCount = 0;
		totalTime = 0.0;
	}

	g_CBuffer.model = XMMatrixIdentity();
	g_CBuffer.view = m_Camera->GetViewMatrix();
	g_CBuffer.projection = m_Camera->GetProjectionMatrix();

	XMFLOAT3 pos = m_Camera->GetPosition();

	g_CBuffer.camPos.x = pos.x;
	g_CBuffer.camPos.y = pos.y;
	g_CBuffer.camPos.z = pos.z;
	g_CBuffer.camPos.w = 1.0f;

	memcpy(m_ConstantBufferGPUAddress[0], &g_CBuffer, sizeof(ConstantBuffer));


	//CAMERA MOVEMENTS
	if (m_CameraMovements[FORWARD])
		m_Camera->ProcessKeyboard(FORWARD, m_DeltaTime);

	if (m_CameraMovements[BACKWARD])
		m_Camera->ProcessKeyboard(BACKWARD, m_DeltaTime);

	if (m_CameraMovements[LEFT])
		m_Camera->ProcessKeyboard(LEFT, m_DeltaTime);

	if (m_CameraMovements[RIGHT])
		m_Camera->ProcessKeyboard(RIGHT, m_DeltaTime);

	if (m_CameraMovements[UP])
		m_Camera->ProcessKeyboard(UP, m_DeltaTime);

	if (m_CameraMovements[DOWN])
		m_Camera->ProcessKeyboard(DOWN, m_DeltaTime);
}

// Transition a resource
void Demo::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
	Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource.Get(),
		beforeState, afterState);

	commandList->ResourceBarrier(1, &barrier);
}

// Clear a render target.
void Demo::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor)
{
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void Demo::ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth)
{
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void Demo::OnRender(RenderEventArgs& e)
{
	super::OnRender(e);

	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList();

#ifdef ENABLE_IMGUI
	ImGui_ImplDX12_NewFrame(commandList.Get());
#endif

	//////////////////////////////////
	//	SHADING PASS ON HDR BUFFER  //
	//////////////////////////////////

	auto rtv = m_HDRHeap->GetCPUDescriptorHandleForHeapStart();
	auto dsv = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

	// Clear the render targets.
	{
		TransitionResource(commandList, m_HDRBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		ClearRTV(commandList, rtv, clearColor);
		ClearDepth(commandList, dsv);
	}

	commandList->SetPipelineState(m_PipelineStates[PipelineStage_ShadingPass].Get());
	commandList->SetGraphicsRootSignature(m_RootSignatures[PipelineStage_ShadingPass].Get());

	ID3D12DescriptorHeap* main_heap = m_DescriptorHeap.Get();
	commandList->SetDescriptorHeaps(1, &main_heap);
	commandList->SetGraphicsRootDescriptorTable(1, m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	commandList->RSSetViewports(1, &m_Viewport);
	commandList->RSSetScissorRects(1, &m_ScissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, VBE_Count, m_VertexBufferViews);
	commandList->IASetIndexBuffer(&m_IndexBufferView);

	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	commandList->SetGraphicsRootConstantBufferView(0, m_ConstantBufferUploadHeap->GetGPUVirtualAddress());

	commandList->DrawIndexedInstanced(m_NumSphereFaces * 3, 1, 0, 0, 0);


	// Transition HDR Target to shader resource view
	{
		TransitionResource(commandList, m_HDRBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}


	UINT currentBackBufferIndex = m_pWindow->GetCurrentBackBufferIndex();
	auto backBuffer = m_pWindow->GetCurrentBackBuffer();
	auto backBufferRTV = m_pWindow->GetCurrentRenderTargetView();

	// Transition back buffer.
	{
		TransitionResource(commandList, backBuffer,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		ClearRTV(commandList, backBufferRTV, clearColor);
	}

	commandList->SetPipelineState(m_PipelineStates[PipelineStage_TonemapPass].Get());
	commandList->SetGraphicsRootSignature(m_RootSignatures[PipelineStage_TonemapPass].Get());

	commandList->SetDescriptorHeaps(1, &main_heap);

	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	handle.Offset(m_ResourcesHeapIncrement * PBR_Count);

	commandList->SetGraphicsRootDescriptorTable(0, handle);

	commandList->RSSetViewports(1, &m_Viewport);
	commandList->RSSetScissorRects(1, &m_ScissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	commandList->OMSetRenderTargets(1, &backBufferRTV, FALSE, NULL);

	commandList->DrawInstanced(4u, 1u, 0u, 0u);

#ifdef ENABLE_IMGUI
	ID3D12DescriptorHeap* imgui_heap = m_ImguiHeap.Get();
	commandList->SetDescriptorHeaps(1, &imgui_heap);

	RenderImgui();

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData());
#endif

	{
		TransitionResource(commandList, backBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		m_FenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

		currentBackBufferIndex = m_pWindow->Present();

		commandQueue->WaitForFenceValue(m_FenceValues[currentBackBufferIndex]);
	}
}

void Demo::OnKeyPressed(KeyEventArgs& e)
{
	super::OnKeyPressed(e);

	static bool controlPressed = false;
	static bool zPressed = false;

	switch (e.Key)
	{
	case KeyCode::Escape:
		Application::Get().Quit(0);
		break;
	case KeyCode::Enter:
		if (e.Alt)
		{
	case KeyCode::F11:
		m_pWindow->ToggleFullscreen();
		break;
		}
	case KeyCode::V:
		m_pWindow->ToggleVSync();
		break;

	case KeyCode::W:
		m_CameraMovements[FORWARD] = true;
		break;

	case KeyCode::S:
		m_CameraMovements[BACKWARD] = true;
		break;

	case KeyCode::A:
		m_CameraMovements[LEFT] = true;
		break;

	case KeyCode::D:
		m_CameraMovements[RIGHT] = true;
		break;

	case KeyCode::Q:
		m_CameraMovements[DOWN] = true;
		break;

	case KeyCode::E:
		m_CameraMovements[UP] = true;
		break;
	}
}

void Demo::OnKeyReleased(KeyEventArgs& e)
{
	super::OnKeyReleased(e);

	switch (e.Key)
	{

	case KeyCode::W:
		m_CameraMovements[FORWARD] = false;
		break;

	case KeyCode::S:
		m_CameraMovements[BACKWARD] = false;
		break;

	case KeyCode::A:
		m_CameraMovements[LEFT] = false;
		break;

	case KeyCode::D:
		m_CameraMovements[RIGHT] = false;
		break;

	case KeyCode::Q:
		m_CameraMovements[DOWN] = false;
		break;

	case KeyCode::E:
		m_CameraMovements[UP] = false;
		break;
	}
}

void Demo::OnMouseWheel(MouseWheelEventArgs& e)
{
}

void Demo::OnMouseMoved(MouseMotionEventArgs & e)
{
	if (m_FirstMouse)
	{
		m_LastX = e.X;
		m_LastY = e.Y;
		m_FirstMouse = false;
	}

	float xoffset = e.X - m_LastX;
	float yoffset = m_LastY - e.Y; // reversed since y-coordinates go from bottom to top

	m_LastX = e.X;
	m_LastY = e.Y;

	if (e.LeftButton)
	{
		m_Camera->ProcessMouseMovement(xoffset, yoffset);
	}
}

void Demo::OnMouseButtonPressed(MouseButtonEventArgs & e)
{
}

void Demo::OnMouseButtonReleased(MouseButtonEventArgs & e)
{
}

void Demo::LoadSphere()
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile("Resources/Meshes/Sphere.obj", aiProcess_Triangulate | aiProcess_CalcTangentSpace |
		aiProcess_GenSmoothNormals);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		assert(false);
	}


	aiMesh* mesh = scene->mMeshes[0];

	m_NumSphereVertices = mesh->mNumVertices;

	for (uint32_t i = 0; i < m_NumSphereVertices; ++i)
	{
		XMFLOAT3& pos = g_VerticesPos[i];

		pos.x = mesh->mVertices[i].x;
		pos.y = mesh->mVertices[i].y;
		pos.z = mesh->mVertices[i].z;

		XMFLOAT2& tc = g_VerticesTexCoords[i];

		tc.x = mesh->mTextureCoords[0][i].x;
		tc.y = mesh->mTextureCoords[0][i].y;

		XMFLOAT3& norm = g_VertNormals[i];

		norm.x = mesh->mNormals[i].x;
		norm.y = mesh->mNormals[i].y;
		norm.z = mesh->mNormals[i].z;

		XMFLOAT3& tan = g_VertTangents[i];

		tan.x = mesh->mTangents[i].x;
		tan.y = mesh->mTangents[i].y;
		tan.z = mesh->mTangents[i].z;

		XMFLOAT3& btan = g_VertBitangents[i];

		btan.x = mesh->mBitangents[i].x;
		btan.y = mesh->mBitangents[i].y;
		btan.z = mesh->mBitangents[i].z;
	}

	m_NumSphereFaces = mesh->mNumFaces;

	for (uint32_t j = 0; j < m_NumSphereFaces; ++j)
	{
		aiFace face = mesh->mFaces[j];

		for (uint16_t f = 0; f < face.mNumIndices; ++f)
		{
			uint32_t& idx = g_Indices[j * 3 + f];

			idx = face.mIndices[f];
		}
	}
}
