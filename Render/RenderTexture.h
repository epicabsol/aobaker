#pragma once

#include <d3d12.h>

class RenderTexture 
{
public:
	RenderTexture(const int& width, const int& height, const DXGI_FORMAT& format);
	~RenderTexture();
	size_t GetIndex() const;
	void UploadData(char* const data, const size_t& length) const;
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;
	void UploadCheckerboard(char r, char g, char b) const;

private:
	int Index; // The index of the SRV descriptor
	int Width;
	int Height;
	DXGI_FORMAT Format;
	ID3D12Resource* GPUResource;
};