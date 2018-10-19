#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include "Render/RenderMesh.h"

// Handles all the global DirectX state.

const float PI = 3.1415926535897932384626433832795f;

namespace Renderer
{
	extern IDXGISwapChain3* SwapChain;
	extern ID3D12Device* Device;
	extern ID3D12CommandAllocator* CommandAllocator;
	extern ID3D12CommandQueue* CommandQueue;
	extern ID3D12DescriptorHeap* RTVHeap;
	extern ID3D12DescriptorHeap* DSVHeap;
	extern ID3D12DescriptorHeap* SRVHeap;

	HRESULT Initialize(const int& bufferWidth, const int& bufferHeight);

	HRESULT Resize(const int& bufferWidth, const int& bufferHeight);

	void BeginScene();

	void EndScene();

	void Present();

	void Dispose();

	ID3DBlob* ReadBlobFromFile(const std::wstring& filename);

	HRESULT UploadData(ID3D12Resource* const destination, void* const data, const size_t& length);

	HRESULT UploadTexture(ID3D12Resource* const destination, void* const data, const size_t& length, const int& width, const int& height, const DXGI_FORMAT& format);

	void SetView(const DirectX::SimpleMath::Matrix& view);

	DirectX::SimpleMath::Matrix GetView();

	DirectX::SimpleMath::Matrix GetProjection();

	void Render(RenderMesh* const mesh, const DirectX::SimpleMath::Matrix& transform);

	size_t AllocateTextureIndex();
}