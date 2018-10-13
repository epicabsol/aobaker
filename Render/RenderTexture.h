#pragma once

#include "../Renderer.h"

class RenderTexture {
public:
	RenderTexture(const int& width, const int& height, const DXGI_FORMAT& format);
	~RenderTexture();
	void UploadData(char* data, size_t length);

private:
	int Width;
	int Height;
	DXGI_FORMAT Format;
	ID3D12Resource* GPUResource;
};