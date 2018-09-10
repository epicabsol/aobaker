#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include "Render/RenderMesh.h"

// Handles all the global DirectX state.

namespace Renderer
{
	extern IDXGISwapChain3* SwapChain;
	extern ID3D12Device* Device;
	extern ID3D12CommandAllocator* CommandAllocator;
	extern ID3D12CommandQueue* CommandQueue;
	extern ID3D12DescriptorHeap* RTVHeap;
	extern ID3D12DescriptorHeap* SRVHeap;

	HRESULT Initialize(const int& bufferWidth, const int& bufferHeight);

	HRESULT Resize(const int& bufferWidth, const int& bufferHeight);

	void BeginScene();

	void EndScene();

	void Present();

	void Dispose();

	ID3DBlob* ReadBlobFromFile(const std::wstring& filename);

	HRESULT UploadData(ID3D12Resource* const destination, void* const data, const size_t& length);

	void SetView(const DirectX::SimpleMath::Matrix& view);

	void Render(RenderMesh* const mesh, const DirectX::SimpleMath::Matrix& transform);
}