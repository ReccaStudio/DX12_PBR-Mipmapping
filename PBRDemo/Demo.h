#pragma once

#include "Game.h"
#include "Window.h"
#include "Camera.h"

#include <string>

#include <DirectXMath.h>

class Demo : public Game
{
public:
	using super = Game;

	Demo(const std::wstring& name, int width, int height, bool vSync = false);
	/**
	*  Load content required for the demo.
	*/
	virtual bool LoadContent() override;

	/**
	*  Unload demo specific content that was loaded in LoadContent.
	*/
	virtual void UnloadContent() override;
protected:
	/**
	*  Update the game logic.
	*/
	virtual void OnUpdate(UpdateEventArgs& e) override;

	/**
	*  Render stuff.
	*/
	virtual void OnRender(RenderEventArgs& e) override;

	/**
	* Invoked by the registered window when a key is pressed
	* while the window has focus.
	*/
	virtual void OnKeyPressed(KeyEventArgs& e) override;

	/**
	* Invoked by the registered window when a key is released
	* while the window has focus.
	*/
	virtual void OnKeyReleased(KeyEventArgs& e) override;

	/**
	* Invoked when the mouse wheel is scrolled while the registered window has focus.
	*/
	virtual void OnMouseWheel(MouseWheelEventArgs& e) override;

	virtual void OnMouseMoved(MouseMotionEventArgs& e) override;

	virtual void OnMouseButtonPressed(MouseButtonEventArgs& e) override;

	virtual void OnMouseButtonReleased(MouseButtonEventArgs& e) override;

	virtual void OnResize(ResizeEventArgs& e) override;

	void LoadSphere();

private:
	// Helper functions
	// Transition a resource
	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	// Clear a render target view.
	void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

	// Clear the depth of a depth-stencil view.
	void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

	// Create a GPU buffer.
	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	// Create a GPU Texture.
	void UpdateTextureResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
		size_t width, size_t height, size_t mipLevels, size_t channels, unsigned char* textureData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	// Resize the depth buffer to match the size of the client area.
	void ResizeDepthBuffer(int width, int height);

	void ResizeRenderTarget(int width, int height);


	void RenderImgui();

	uint64_t m_FenceValues[Window::BufferCount] = {};


	enum VertexBufferElement
	{
		VBE_Position = 0,
		VBE_TexCoord,
		VBE_Normals,
		VBE_Tangents,
		VBE_Bitangents,

		VBE_Count
	};

	enum PBRMaterialElements
	{
		PBR_Albedo = 0,
		PBR_Normal,
		PBR_MetallicRoughness,

		PBR_Count
	};

	enum PipelineStages
	{
		PipelineStage_ShadingPass = 0,
		PipelineStage_TonemapPass,

		PipelineState_Count
	};

	// Vertex buffer for the sphere.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffers[VBE_Count];
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferViews[VBE_Count];
	// Index buffer for the sphere.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ConstantBufferUploadHeap;
	UINT8* m_ConstantBufferGPUAddress[1];

	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	// Depth buffer.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;
	// Descriptor heap for depth buffer.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

	// HDR RenderTarget
	Microsoft::WRL::ComPtr<ID3D12Resource> m_HDRBuffer;
	// Descriptor heap for the render target.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_HDRHeap;
	// SRV for the render target
	D3D12_SHADER_RESOURCE_VIEW_DESC m_HDRBufferView;

	UINT m_HDRHandleIncrement;

	// Main descriptor heap.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;

	UINT m_ResourcesHeapIncrement;

	// Descriptor heap for ImGui.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_ImguiHeap;

	// Root signature
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignatures[PipelineState_Count];

	// Pipeline state object.
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineStates[PipelineState_Count];

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	DirectX::XMMATRIX m_ModelMatrix;

	//Textures
	Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureResource[PBR_Count];
	D3D12_SHADER_RESOURCE_VIEW_DESC m_TextureResourceView[PBR_Count];

	//Spheres values
	uint32_t m_NumSphereVertices;
	uint32_t m_NumSphereFaces;

	Camera* m_Camera;
	float m_LastX, m_LastY;
	bool m_FirstMouse = true;

	bool m_CameraMovements[6] = { false, false, false, false, false, false };

	bool m_ContentLoaded;

	float m_DeltaTime;
};

