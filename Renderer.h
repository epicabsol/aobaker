#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <string>
#include "Render/RenderMesh.h"

// Handles all the global DirectX state.

const float PI = 3.1415926535897932384626433832795f;

namespace Renderer
{
	extern IDXGISwapChain* SwapChain;
	extern ID3D11Device* Device;
	extern ID3D11DeviceContext* ImmediateContext;

	HRESULT Initialize(const int& bufferWidth, const int& bufferHeight);

	HRESULT Resize(const int& bufferWidth, const int& bufferHeight);

	void BeginScene();

	void EndScene();

	void Present();

	void Dispose();

	ID3DBlob* ReadBlobFromFile(const std::wstring& filename);

	void SetView(const DirectX::SimpleMath::Matrix& view);

	DirectX::SimpleMath::Matrix GetView();

	DirectX::SimpleMath::Matrix GetProjection();

	void Render(RenderMesh* const mesh, const DirectX::SimpleMath::Matrix& transform);
}